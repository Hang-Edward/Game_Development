using UnityEngine;
using UnityEngine.InputSystem;

public class PlayerController : MonoBehaviour
{
    [Header("Movement")]
    [SerializeField] private float walkSpeed = 6f;
    [SerializeField] private float sprintSpeed = 10f;
    [SerializeField] private float crouchSpeed = 3f;
    [SerializeField] private float blockSpeed = 2f;
    [SerializeField] private float jumpHeight = 1.5f;
    [SerializeField] private float gravity = -25f;
    [SerializeField] private float groundSnapDistance = 200f;
    [SerializeField] private float groundStickDistance = 0.35f;
    [SerializeField] private float jumpGroundIgnoreTime = 0.18f;
    [SerializeField] private float coyoteTime = 0.12f;
    [SerializeField] private float locomotionGroundGraceTime = 0.08f;
    [SerializeField] private float groundClampOffset = 0.03f;
    [SerializeField] private LayerMask groundMask = ~0;

    [Header("Visual Grounding")]
    [SerializeField] private float walkVisualLift = 0.85f;
    [SerializeField] private float runVisualLift = 0.65f;
    [SerializeField] private float visualLiftSmoothSpeed = 14f;

    [Header("References")]
    [SerializeField] private Animator animator;
    [SerializeField] private Camera playerCamera;

    private CharacterController characterController;
    private Transform animatedModelRoot;
    private Vector3 animatedModelLocalPosition;
    private Quaternion animatedModelLocalRotation;
    private SkinnedMeshRenderer[] animatedRenderers;

    private Vector3 moveDirection;
    private Vector3 velocity;
    private float currentSpeed;
    private float currentVisualLift;
    private float jumpVisualLift;
    private bool grounded;
    private bool sprint;
    private bool crouching;
    private bool blocking;
    private bool hasMoveInput;
    private bool jumping;
    private float jumpGroundIgnoreTimer;
    private float lastGroundedTimer;
    private float locomotionGroundGraceTimer;
    private Vector3 lockedAirMoveDirection;

    private float attackCooldown = 0.35f;
    private float attackTimer;

    private void Awake()
    {
        characterController = GetComponent<CharacterController>();
        if (animator == null) animator = GetComponentInChildren<Animator>();
        if (playerCamera == null) playerCamera = Camera.main;
        if (animator != null) animator.applyRootMotion = false;
        if (animator != null && animator.transform != transform)
        {
            animatedModelRoot = animator.transform;
            animatedModelLocalPosition = animatedModelRoot.localPosition;
            animatedModelLocalRotation = animatedModelRoot.localRotation;
            animatedRenderers = animatedModelRoot.GetComponentsInChildren<SkinnedMeshRenderer>(true);
            foreach (var renderer in animatedRenderers)
            {
                renderer.updateWhenOffscreen = true;
                renderer.localBounds = new Bounds(Vector3.up, Vector3.one * 6f);
            }
        }

        ConfigureCharacterController();
    }

    private void Start()
    {
        TerrainCollisionBuilder.EnsureSceneTerrainColliders();
        SnapToGround();
    }

    private void Update()
    {
        RefreshGroundedBeforeInput();

        // Read input directly from keyboard/mouse
        Vector2 moveInput = Vector2.zero;
        var kb = Keyboard.current;
        if (kb == null) return;

        if (kb.wKey.isPressed) moveInput.y = 1;
        if (kb.sKey.isPressed) moveInput.y = -1;
        if (kb.aKey.isPressed) moveInput.x = -1;
        if (kb.dKey.isPressed) moveInput.x = 1;

        crouching = kb.leftCtrlKey.isPressed;
        sprint = kb.leftShiftKey.isPressed && !crouching;
        blocking = Mouse.current != null && Mouse.current.rightButton.isPressed;

        // Attack
        if (Mouse.current != null && Mouse.current.leftButton.wasPressedThisFrame && attackTimer <= 0 && !blocking)
        {
            attackTimer = attackCooldown;
            if (animator != null) animator.SetTrigger("Attack");
        }

        // Calculate speed
        if (blocking) currentSpeed = blockSpeed;
        else if (crouching) currentSpeed = crouchSpeed;
        else if (sprint) currentSpeed = sprintSpeed;
        else currentSpeed = walkSpeed;

        // Calculate move direction relative to camera
        Vector3 forward = playerCamera.transform.forward;
        Vector3 right = playerCamera.transform.right;
        forward.y = 0; right.y = 0;
        forward.Normalize(); right.Normalize();

        Vector3 desiredMove = forward * moveInput.y + right * moveInput.x;
        if (desiredMove.magnitude > 1f) desiredMove.Normalize();
        hasMoveInput = desiredMove.sqrMagnitude > 0.01f;

        if (grounded)
        {
            moveDirection = desiredMove * currentSpeed;
        }
        else if (jumping)
        {
            if (lockedAirMoveDirection.sqrMagnitude <= 0.0001f && hasMoveInput)
            {
                lockedAirMoveDirection = desiredMove * currentSpeed;
                jumpVisualLift = Mathf.Max(jumpVisualLift, GetVisualLiftForLocomotion(GetLocomotionBlend()));
            }

            moveDirection = lockedAirMoveDirection;
        }
        else
        {
            moveDirection = desiredMove * currentSpeed;
        }

        // Jump
        if (kb.spaceKey.wasPressedThisFrame && CanJump())
        {
            lockedAirMoveDirection = hasMoveInput ? desiredMove * currentSpeed : Vector3.zero;
            jumpVisualLift = Mathf.Max(currentVisualLift, GetTargetVisualLift());

            moveDirection = lockedAirMoveDirection;
            velocity.y = Mathf.Sqrt(jumpHeight * -2f * gravity);
            grounded = false;
            jumping = true;
            jumpGroundIgnoreTimer = jumpGroundIgnoreTime;
            lastGroundedTimer = 0f;
        }

        // Apply horizontal movement
        characterController.Move(moveDirection * Time.deltaTime);

        // Rotate character to face movement direction
        if (desiredMove.magnitude > 0.1f)
        {
            Quaternion targetRotation = Quaternion.LookRotation(desiredMove);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRotation, Time.deltaTime * 12f);
        }

        // Gravity
        if (jumpGroundIgnoreTimer > 0f)
            jumpGroundIgnoreTimer -= Time.deltaTime;

        if (!jumping && grounded && velocity.y < 0)
            velocity.y = -2f;

        velocity.y += gravity * Time.deltaTime;
        CollisionFlags verticalCollision = characterController.Move(velocity * Time.deltaTime);
        ResolveGroundedAfterVerticalMove(verticalCollision);
        ClampAboveGround();

        // Attack cooldown
        if (attackTimer > 0) attackTimer -= Time.deltaTime;

        // Animator
        if (animator != null)
        {
            float locomotionBlend = GetLocomotionBlend();
            animator.SetFloat("Speed", locomotionBlend, 0.08f, Time.deltaTime);
            animator.SetBool("Sprint", sprint && hasMoveInput && !crouching && !blocking);
            animator.SetBool("Crouch", crouching);
            animator.SetBool("Block", blocking);
            animator.SetBool("Grounded", grounded);
            animator.SetFloat("MotionSpeed", locomotionBlend);
        }

    }

    private void ConfigureCharacterController()
    {
        if (characterController == null)
            return;

        characterController.height = Mathf.Max(characterController.height, 1.8f);
        characterController.radius = Mathf.Max(characterController.radius, 0.35f);
        characterController.center = new Vector3(0f, characterController.height * 0.5f, 0f);
        characterController.slopeLimit = 55f;
        characterController.stepOffset = Mathf.Clamp(0.35f, 0.01f, characterController.height - 0.01f);
        characterController.skinWidth = Mathf.Max(characterController.skinWidth, 0.08f);
        groundSnapDistance = Mathf.Max(groundSnapDistance, 200f);
    }

    private void LateUpdate()
    {
        if (animatedModelRoot == null)
            return;

        float targetVisualLift = GetTargetVisualLift();
        currentVisualLift = Mathf.MoveTowards(currentVisualLift, targetVisualLift, visualLiftSmoothSpeed * Time.deltaTime);
        animatedModelRoot.localPosition = animatedModelLocalPosition + Vector3.up * currentVisualLift;
        animatedModelRoot.localRotation = animatedModelLocalRotation;
    }

    private float GetTargetVisualLift()
    {
        if (jumping)
            return jumpVisualLift;

        float locomotion = hasMoveInput ? GetLocomotionBlend() : 0f;
        if (animator != null)
            locomotion = animator.GetFloat("Speed");

        return GetVisualLiftForLocomotion(locomotion);
    }

    private float GetVisualLiftForLocomotion(float locomotion)
    {
        if (locomotion <= 0.01f)
            return 0f;

        if (locomotion <= 0.55f)
            return Mathf.Lerp(0f, walkVisualLift, locomotion / 0.55f);

        return Mathf.Lerp(walkVisualLift, runVisualLift, Mathf.InverseLerp(0.55f, 1f, locomotion));
    }

    private void SnapToGround()
    {
        if (characterController == null)
            return;

        if (!TryGetWalkableGroundAt(transform.position, out RaycastHit hit))
            return;

        characterController.enabled = false;
        transform.position = new Vector3(transform.position.x, hit.point.y + groundClampOffset, transform.position.z);
        characterController.enabled = true;
        velocity.y = -2f;
        grounded = true;
        jumping = false;
    }

    private bool ProbeGround(out RaycastHit hit)
    {
        hit = default;
        float radius = characterController != null ? characterController.radius * 0.9f : 0.3f;
        Vector3 origin = transform.position + Vector3.up * 0.15f;
        RaycastHit[] hits;

        if (characterController == null)
        {
            hits = Physics.SphereCastAll(origin, radius, Vector3.down, groundStickDistance, groundMask, QueryTriggerInteraction.Ignore);
        }
        else
        {
            bool wasEnabled = characterController.enabled;
            characterController.enabled = false;
            hits = Physics.SphereCastAll(origin, radius, Vector3.down, groundStickDistance, groundMask, QueryTriggerInteraction.Ignore);
            characterController.enabled = wasEnabled;
        }

        return TryPickHighestWalkableHit(hits, out hit);
    }

    private void ClampAboveGround()
    {
        if (jumping || !CanEvaluateGround() || velocity.y > 0f)
            return;

        if (!TryGetWalkableGroundAt(transform.position, out RaycastHit hit))
            return;

        float minFeetY = hit.point.y + groundClampOffset;
        if (transform.position.y >= minFeetY)
            return;

        characterController.enabled = false;
        transform.position = new Vector3(transform.position.x, minFeetY, transform.position.z);
        characterController.enabled = true;

        if (velocity.y < 0f)
            velocity.y = -2f;

        grounded = true;
        jumping = false;
        lockedAirMoveDirection = Vector3.zero;
        lastGroundedTimer = coyoteTime;
    }

    private bool TryGetWalkableGroundAt(Vector3 position, out RaycastHit bestHit)
    {
        bestHit = default;
        float probeHeight = Mathf.Max(groundSnapDistance, characterController != null ? characterController.height * 10f : 20f);
        Vector3 origin = new Vector3(position.x, position.y + probeHeight, position.z);
        float probeDistance = probeHeight * 2f;
        float maxGroundY = position.y + (characterController != null ? characterController.stepOffset + characterController.skinWidth : 0.5f);
        RaycastHit[] hits;

        if (characterController == null)
        {
            hits = Physics.RaycastAll(origin, Vector3.down, probeDistance, groundMask, QueryTriggerInteraction.Ignore);
        }
        else
        {
            bool wasEnabled = characterController.enabled;
            characterController.enabled = false;
            hits = Physics.RaycastAll(origin, Vector3.down, probeDistance, groundMask, QueryTriggerInteraction.Ignore);
            characterController.enabled = wasEnabled;
        }

        return TryPickHighestWalkableHit(hits, maxGroundY, out bestHit);
    }

    private bool TryPickHighestWalkableHit(RaycastHit[] hits, out RaycastHit bestHit)
    {
        return TryPickHighestWalkableHit(hits, float.PositiveInfinity, out bestHit);
    }

    private bool TryPickHighestWalkableHit(RaycastHit[] hits, float maxGroundY, out RaycastHit bestHit)
    {
        bestHit = default;
        bool found = false;
        float highestY = float.NegativeInfinity;

        foreach (var candidate in hits)
        {
            if (!IsWalkableGround(candidate))
                continue;

            if (candidate.point.y > maxGroundY)
                continue;

            if (candidate.point.y <= highestY)
                continue;

            highestY = candidate.point.y;
            bestHit = candidate;
            found = true;
        }

        return found;
    }

    private bool IsWalkableGround(RaycastHit hit)
    {
        if (hit.collider == null || hit.collider.transform.root == transform.root)
            return false;

        if (hit.normal.y <= 0.01f)
            return false;

        float slopeLimit = characterController != null ? characterController.slopeLimit : 55f;
        return Vector3.Angle(hit.normal, Vector3.up) <= slopeLimit;
    }

    private bool CanEvaluateGround()
    {
        return jumpGroundIgnoreTimer <= 0f;
    }

    private bool CanJump()
    {
        return !crouching && !jumping && (grounded || lastGroundedTimer > 0f);
    }

    private float GetLocomotionBlend()
    {
        if (!hasMoveInput)
            return 0f;

        if (sprint && !crouching && !blocking)
            return 1f;

        return 0.55f;
    }

    private void RefreshGroundedBeforeInput()
    {
        if (jumping)
        {
            grounded = false;
            return;
        }

        bool detectedGround = CanEvaluateGround() && (characterController.isGrounded || ProbeGround(out _));
        if (detectedGround)
        {
            grounded = true;
            lastGroundedTimer = coyoteTime;
            locomotionGroundGraceTimer = locomotionGroundGraceTime;
            return;
        }

        if (hasMoveInput && velocity.y <= 0f && locomotionGroundGraceTimer > 0f)
        {
            locomotionGroundGraceTimer -= Time.deltaTime;
            grounded = true;
            lastGroundedTimer = coyoteTime;
            return;
        }

        grounded = false;
        if (lastGroundedTimer > 0f)
            lastGroundedTimer -= Time.deltaTime;
    }

    private void ResolveGroundedAfterVerticalMove(CollisionFlags verticalCollision)
    {
        bool touchedGround = (verticalCollision & CollisionFlags.Below) != 0 || characterController.isGrounded;

        if (jumping)
        {
            if (velocity.y <= 0f && touchedGround)
            {
                jumping = false;
                grounded = true;
                velocity.y = -2f;
                lockedAirMoveDirection = Vector3.zero;
                jumpVisualLift = 0f;
                lastGroundedTimer = coyoteTime;
            }
            else
            {
                grounded = false;
            }

            return;
        }

        bool detectedGround = touchedGround || (CanEvaluateGround() && ProbeGround(out _));
        if (detectedGround)
        {
            grounded = true;
            lastGroundedTimer = coyoteTime;
            locomotionGroundGraceTimer = locomotionGroundGraceTime;
            return;
        }

        if (hasMoveInput && velocity.y <= 0f && locomotionGroundGraceTimer > 0f)
        {
            locomotionGroundGraceTimer -= Time.deltaTime;
            grounded = true;
            lastGroundedTimer = coyoteTime;
            return;
        }

        grounded = false;
    }
}

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
    [SerializeField] private float jumpGroundIgnoreTime = 0.12f;
    [SerializeField] private float coyoteTime = 0.12f;
    [SerializeField] private float groundClampOffset = 0.03f;
    [SerializeField] private float visualGroundOffset = 0.02f;
    [SerializeField] private float visualRescueThreshold = 0.25f;
    [SerializeField] private LayerMask groundMask = ~0;

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
    private bool grounded;
    private bool sprint;
    private bool crouching;
    private bool blocking;
    private bool hasMoveInput;
    private bool jumping;
    private float jumpGroundIgnoreTimer;
    private float lastGroundedTimer;
    private Vector3 lockedAirMoveDirection;
    private Vector3 lastGroundedMoveDirection;

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
        else if (sprint && grounded) currentSpeed = sprintSpeed;
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
            lastGroundedMoveDirection = moveDirection;
        }
        else if (jumping)
        {
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
            animator.SetBool("Sprint", sprint && grounded);
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

        animatedModelRoot.localPosition = animatedModelLocalPosition;
        animatedModelRoot.localRotation = animatedModelLocalRotation;
        KeepVisibleModelAboveControllerFeet();
    }

    private void KeepVisibleModelAboveControllerFeet()
    {
        if (animatedRenderers == null || animatedRenderers.Length == 0)
            return;

        bool hasBounds = false;
        Bounds combinedBounds = default;
        foreach (var renderer in animatedRenderers)
        {
            if (renderer == null || !renderer.enabled)
                continue;

            if (!hasBounds)
            {
                combinedBounds = renderer.bounds;
                hasBounds = true;
            }
            else
            {
                combinedBounds.Encapsulate(renderer.bounds);
            }
        }

        if (!hasBounds)
            return;

        float targetY = transform.position.y + visualGroundOffset;
        if (TryGetHighestGroundBelow(out RaycastHit hit))
            targetY = Mathf.Max(targetY, hit.point.y + visualGroundOffset);

        if (combinedBounds.max.y >= targetY + visualRescueThreshold)
            return;

        Vector3 localPosition = animatedModelRoot.localPosition;
        localPosition.y += targetY - combinedBounds.min.y;
        animatedModelRoot.localPosition = localPosition;
    }

    private void SnapToGround()
    {
        if (characterController == null)
            return;

        if (!TryGetHighestGroundBelow(out RaycastHit hit))
            return;

        characterController.enabled = false;
        transform.position = hit.point;
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
        if (!TryGetHighestGroundBelow(out RaycastHit hit))
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

    private bool TryGetHighestGroundBelow(out RaycastHit bestHit)
    {
        bestHit = default;
        Vector3 origin = transform.position + Vector3.up * groundSnapDistance;
        RaycastHit[] hits;

        if (characterController == null)
        {
            hits = Physics.RaycastAll(origin, Vector3.down, groundSnapDistance * 2f, groundMask, QueryTriggerInteraction.Ignore);
        }
        else
        {
            bool wasEnabled = characterController.enabled;
            characterController.enabled = false;
            hits = Physics.RaycastAll(origin, Vector3.down, groundSnapDistance * 2f, groundMask, QueryTriggerInteraction.Ignore);
            characterController.enabled = wasEnabled;
        }

        return TryPickHighestGroundHit(hits, out bestHit);
    }

    private bool TryPickHighestGroundHit(RaycastHit[] hits, out RaycastHit bestHit)
    {
        bestHit = default;
        bool found = false;
        float highestY = float.NegativeInfinity;

        foreach (var candidate in hits)
        {
            if (candidate.collider == null || candidate.collider.transform.root == transform.root)
                continue;

            if (candidate.point.y <= highestY)
                continue;

            highestY = candidate.point.y;
            bestHit = candidate;
            found = true;
        }

        return found;
    }

    private bool TryPickHighestWalkableHit(RaycastHit[] hits, out RaycastHit bestHit)
    {
        bestHit = default;
        bool found = false;
        float highestY = float.NegativeInfinity;

        foreach (var candidate in hits)
        {
            if (!IsWalkableGround(candidate))
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

        if (sprint && grounded && !crouching && !blocking)
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

        grounded = CanEvaluateGround() && (characterController.isGrounded || ProbeGround(out _));
        if (grounded)
            lastGroundedTimer = coyoteTime;
        else if (lastGroundedTimer > 0f)
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
                lastGroundedTimer = coyoteTime;
            }
            else
            {
                grounded = false;
            }

            return;
        }

        grounded = touchedGround || (CanEvaluateGround() && ProbeGround(out _));
        if (grounded)
            lastGroundedTimer = coyoteTime;
    }
}

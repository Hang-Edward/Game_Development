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
    [SerializeField] private float groundSnapDistance = 50f;
    [SerializeField] private float groundStickDistance = 0.75f;
    [SerializeField] private float jumpGroundIgnoreTime = 0.12f;
    [SerializeField] private float coyoteTime = 0.12f;
    [SerializeField] private LayerMask groundMask = ~0;

    [Header("References")]
    [SerializeField] private Animator animator;
    [SerializeField] private Camera playerCamera;

    private CharacterController characterController;

    private Vector3 moveDirection;
    private Vector3 velocity;
    private float currentSpeed;
    private bool grounded;
    private bool sprint;
    private bool crouching;
    private bool blocking;
    private bool jumping;
    private float jumpGroundIgnoreTimer;
    private float lastGroundedTimer;

    private float attackCooldown = 0.35f;
    private float attackTimer;

    private void Awake()
    {
        characterController = GetComponent<CharacterController>();
        if (animator == null) animator = GetComponentInChildren<Animator>();
        if (playerCamera == null) playerCamera = Camera.main;

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

        sprint = kb.leftShiftKey.isPressed && !crouching;
        crouching = kb.leftCtrlKey.isPressed;
        blocking = Mouse.current != null && Mouse.current.rightButton.isPressed;

        // Jump
        if (kb.spaceKey.wasPressedThisFrame && CanJump())
        {
            velocity.y = Mathf.Sqrt(jumpHeight * -2f * gravity);
            grounded = false;
            jumping = true;
            jumpGroundIgnoreTimer = jumpGroundIgnoreTime;
            lastGroundedTimer = 0f;
        }

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
        moveDirection = desiredMove * currentSpeed;

        // Apply horizontal movement
        characterController.Move(moveDirection * Time.deltaTime);
        StickToGround();

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

        // Attack cooldown
        if (attackTimer > 0) attackTimer -= Time.deltaTime;

        // Animator
        if (animator != null)
        {
            float speed = new Vector3(characterController.velocity.x, 0, characterController.velocity.z).magnitude;
            animator.SetFloat("Speed", speed);
            animator.SetBool("Sprint", sprint && grounded);
            animator.SetBool("Crouch", crouching);
            animator.SetBool("Block", blocking);
            animator.SetBool("Grounded", grounded);
            animator.SetFloat("MotionSpeed", speed / sprintSpeed);
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
    }

    private void SnapToGround()
    {
        if (characterController == null)
            return;

        characterController.enabled = false;
        bool foundGround = Physics.Raycast(transform.position + Vector3.up * 5f, Vector3.down, out RaycastHit hit, groundSnapDistance, groundMask, QueryTriggerInteraction.Ignore);
        characterController.enabled = true;

        if (!foundGround)
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
        float radius = characterController != null ? characterController.radius * 0.9f : 0.3f;
        Vector3 origin = transform.position + Vector3.up * 0.15f;
        if (characterController == null)
            return Physics.SphereCast(origin, radius, Vector3.down, out hit, groundStickDistance, groundMask, QueryTriggerInteraction.Ignore);

        bool wasEnabled = characterController.enabled;
        characterController.enabled = false;
        bool foundGround = Physics.SphereCast(origin, radius, Vector3.down, out hit, groundStickDistance, groundMask, QueryTriggerInteraction.Ignore);
        characterController.enabled = wasEnabled;
        return foundGround;
    }

    private void StickToGround()
    {
        if (!CanStickToGround())
            return;

        if (!ProbeGround(out RaycastHit hit))
            return;

        float slope = Vector3.Angle(hit.normal, Vector3.up);
        if (slope > characterController.slopeLimit)
            return;

        float delta = transform.position.y - hit.point.y;
        if (delta > 0.001f && delta < groundStickDistance)
            characterController.Move(Vector3.down * delta);
    }

    private bool CanStickToGround()
    {
        return !jumping && jumpGroundIgnoreTimer <= 0f && velocity.y <= 0f;
    }

    private bool CanEvaluateGround()
    {
        return jumpGroundIgnoreTimer <= 0f;
    }

    private bool CanJump()
    {
        return !crouching && !jumping && (grounded || lastGroundedTimer > 0f);
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

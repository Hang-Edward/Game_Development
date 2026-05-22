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

    private float attackCooldown = 0.35f;
    private float attackTimer;

    private void Awake()
    {
        characterController = GetComponent<CharacterController>();
        if (animator == null) animator = GetComponentInChildren<Animator>();
        if (playerCamera == null) playerCamera = Camera.main;
    }

    private void Update()
    {
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
        if (kb.spaceKey.wasPressedThisFrame && grounded && !crouching)
            velocity.y = Mathf.Sqrt(jumpHeight * -2f * gravity);

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

        // Rotate character to face movement direction
        if (desiredMove.magnitude > 0.1f)
        {
            Quaternion targetRotation = Quaternion.LookRotation(desiredMove);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRotation, Time.deltaTime * 12f);
        }

        // Gravity
        grounded = characterController.isGrounded;
        if (grounded && velocity.y < 0) velocity.y = -2f;
        velocity.y += gravity * Time.deltaTime;
        characterController.Move(velocity * Time.deltaTime);

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
}

using UnityEngine;

// Manages animation state and provides helper methods for the Animator Controller
// Designed to work with the CharacterAnimator.controller blend tree
public class AnimationStateController : MonoBehaviour
{
    private Animator animator;

    // Animation parameter hashes for efficiency
    private static readonly int SpeedHash = Animator.StringToHash("Speed");
    private static readonly int SprintHash = Animator.StringToHash("Sprint");
    private static readonly int CrouchHash = Animator.StringToHash("Crouch");
    private static readonly int BlockHash = Animator.StringToHash("Block");
    private static readonly int GroundedHash = Animator.StringToHash("Grounded");
    private static readonly int AttackHash = Animator.StringToHash("Attack");
    private static readonly int MotionSpeedHash = Animator.StringToHash("MotionSpeed");

    private void Awake()
    {
        animator = GetComponent<Animator>();
        if (animator == null)
            animator = GetComponentInParent<Animator>();
    }

    public void SetSpeed(float speed)
    {
        if (animator != null) animator.SetFloat(SpeedHash, speed);
    }

    public void SetSprint(bool value)
    {
        if (animator != null) animator.SetBool(SprintHash, value);
    }

    public void SetCrouch(bool value)
    {
        if (animator != null) animator.SetBool(CrouchHash, value);
    }

    public void SetBlock(bool value)
    {
        if (animator != null) animator.SetBool(BlockHash, value);
    }

    public void SetGrounded(bool value)
    {
        if (animator != null) animator.SetBool(GroundedHash, value);
    }

    public void TriggerAttack()
    {
        if (animator != null) animator.SetTrigger(AttackHash);
    }

    public void SetMotionSpeed(float speed)
    {
        if (animator != null) animator.SetFloat(MotionSpeedHash, speed);
    }
}

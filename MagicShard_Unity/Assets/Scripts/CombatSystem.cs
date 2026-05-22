using UnityEngine;

// Handles attack logic, cooldowns, and hit detection
// Replaces main.cpp's atkTimer, swingAnim, hitThisAttack variables
public class CombatSystem : MonoBehaviour
{
    [Header("Attack")]
    [SerializeField] private float attackCooldown = 0.35f;
    [SerializeField] private float attackRange = 2f;

    [Header("References")]
    [SerializeField] private Animator animator;
    [SerializeField] private Transform weaponTip;
    [SerializeField] private LayerMask hitLayer;

    private float currentCooldown;
    private bool hitThisAttack;

    private void Awake()
    {
        if (animator == null)
            animator = GetComponentInChildren<Animator>();
    }

    private void Update()
    {
        if (currentCooldown > 0)
            currentCooldown -= Time.deltaTime;
    }

    public bool CanAttack()
    {
        return currentCooldown <= 0;
    }

    public void PerformAttack()
    {
        if (!CanAttack()) return;

        currentCooldown = attackCooldown;
        hitThisAttack = false;

        if (animator != null)
            animator.SetTrigger("Attack");
    }

    // Called by animation event (weapon swing midpoint)
    public void OnAttackHitDetection()
    {
        if (hitThisAttack) return;

        // Sphere cast from weapon tip forward
        if (weaponTip != null)
        {
            Collider[] hits = Physics.OverlapSphere(weaponTip.position, attackRange, hitLayer);
            foreach (var hit in hits)
            {
                // Don't hit self
                if (hit.transform.root == transform.root) continue;

                hitThisAttack = true;
                Debug.Log($"Hit: {hit.transform.name}");

                // TODO: apply damage via health system
                // var health = hit.GetComponent<IHealth>();
                // if (health) health.TakeDamage(attackDamage);

                break;
            }
        }
        else
        {
            // Fallback: box in front of player
            Vector3 origin = transform.position + transform.forward * 1f;
            Collider[] hits = Physics.OverlapBox(origin, Vector3.one * 0.5f, transform.rotation, hitLayer);
            foreach (var hit in hits)
            {
                if (hit.transform.root == transform.root) continue;
                hitThisAttack = true;
                Debug.Log($"Hit (fallback): {hit.transform.name}");
                break;
            }
        }
    }

    private void OnDrawGizmosSelected()
    {
        if (weaponTip != null)
        {
            Gizmos.color = Color.red;
            Gizmos.DrawWireSphere(weaponTip.position, attackRange);
        }
    }
}

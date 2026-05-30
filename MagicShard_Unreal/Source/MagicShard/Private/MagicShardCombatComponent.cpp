#include "MagicShardCombatComponent.h"

#include "DrawDebugHelpers.h"
#include "MagicShardBaseCharacter.h"

UMagicShardCombatComponent::UMagicShardCombatComponent()
    : AttackDamage(20.0f),
      AttackRange(180.0f),
      AttackCooldown(0.45f),
      CooldownRemaining(0.0f),
      OwnerCharacter(nullptr)
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UMagicShardCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCharacter = Cast<AMagicShardBaseCharacter>(GetOwner());
}

void UMagicShardCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CooldownRemaining > 0.0f)
    {
        CooldownRemaining = FMath::Max(0.0f, CooldownRemaining - DeltaTime);
    }
}

bool UMagicShardCombatComponent::TryMeleeAttack()
{
    if (!CanAttack())
    {
        return false;
    }

    CooldownRemaining = AttackCooldown;
    if (OwnerCharacter != nullptr)
    {
        OwnerCharacter->StartPrimaryAction();
    }

    FHitResult Hit;
    bool bHit = FindAttackTarget(Hit);

    if (bHit)
    {
        AActor* Target = Hit.GetActor();
        if (Target != nullptr && Target != GetOwner())
        {
            if (AMagicShardBaseCharacter* TargetCharacter = Cast<AMagicShardBaseCharacter>(Target))
            {
                TargetCharacter->ReceiveDamage(AttackDamage);
            }
            OnDamageApplied.Broadcast(Target, AttackDamage);
        }
    }

    // 无论攻击是否击中，都必须结束攻击动作状态
    // 否则 ActionState 将永远卡在 Attack，导致 UpdateActionState 失效
    if (OwnerCharacter != nullptr)
    {
        OwnerCharacter->StopPrimaryAction();
    }

    return bHit;
}

bool UMagicShardCombatComponent::CanAttack() const
{
    return OwnerCharacter != nullptr && CooldownRemaining <= 0.0f;
}

void UMagicShardCombatComponent::SetAttackDamage(float NewDamage)
{
    AttackDamage = FMath::Max(0.0f, NewDamage);
}

bool UMagicShardCombatComponent::FindAttackTarget(FHitResult& OutHit) const
{
    if (OwnerCharacter == nullptr || GetWorld() == nullptr)
    {
        return false;
    }

    const FVector Start = OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
    const FVector End = Start + OwnerCharacter->GetActorForwardVector() * AttackRange;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(MagicShardMelee), false, OwnerCharacter);
    return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Pawn, Params);
}

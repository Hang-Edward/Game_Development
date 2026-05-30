#include "MagicShardAnimInstance.h"
#include "MagicShardBaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UMagicShardAnimInstance::UMagicShardAnimInstance()
    : Speed(0.0f)
    , ActionState(EMagicShardActionState::Idle)
    , bIsInAir(false)
    , CurrentBlendSpeed(0.0f)
{
}

void UMagicShardAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    CurrentBlendSpeed = 0.0f;
    Speed = 0.0f;
}

void UMagicShardAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // 获取角色
    AMagicShardBaseCharacter* Character = Cast<AMagicShardBaseCharacter>(TryGetPawnOwner());
    if (Character == nullptr)
    {
        Speed = 0.0f;
        return;
    }

    // 读取角色状态
    ActionState = Character->GetActionState();
    const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    bIsInAir = (MoveComp != nullptr && !MoveComp->IsMovingOnGround());

    // 根据动作状态确定目标混合值
    // 这些值与 BlendSpace 中样本点的 X 坐标对应:
    //   Speed=0.0  → Idle
    //   Speed=0.55 → Walk
    //   Speed=1.0  → Run
    float TargetSpeed = 0.0f;
    switch (ActionState)
    {
        case EMagicShardActionState::Walk:
            TargetSpeed = 0.55f;
            break;
        case EMagicShardActionState::Run:
            TargetSpeed = 1.0f;
            break;
        default:
            TargetSpeed = 0.0f;
            break;
    }

    // 平滑过渡，类似 Unity 的 SmoothDamp(current, target, ref vel, 0.08)
    // FInterpTo 在 InterpSpeed=12 时 ≈ 0.08s 到达目标
    CurrentBlendSpeed = FMath::FInterpTo(CurrentBlendSpeed, TargetSpeed, DeltaSeconds, 12.0f);
    Speed = CurrentBlendSpeed;
}

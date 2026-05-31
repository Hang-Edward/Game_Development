#include "MagicShardBaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

AMagicShardBaseCharacter::AMagicShardBaseCharacter()
    : MaxHealth(120.0f),
      Health(120.0f),
      MaxMana(80.0f),
      Mana(80.0f),
      WalkSpeed(450.0f),
      SprintSpeed(750.0f),
      ActionState(EMagicShardActionState::Idle),
      bSprinting(false)
{
    PrimaryActorTick.bCanEverTick = true;

    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    MoveComp->MaxWalkSpeed = WalkSpeed;
    MoveComp->JumpZVelocity = 520.0f;
    MoveComp->AirControl = 0.85f;
    MoveComp->AirControlBoostMultiplier = 1.0f;
    MoveComp->GravityScale = 1.35f;
    MoveComp->MaxStepHeight = 45.0f;
    MoveComp->SetWalkableFloorAngle(50.0f);
    MoveComp->bOrientRotationToMovement = true;
    MoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
}

void AMagicShardBaseCharacter::BeginPlay()
{
    Super::BeginPlay();
    UpdateMovementSpeed();
}

void AMagicShardBaseCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateActionState(DeltaSeconds);
}

void AMagicShardBaseCharacter::AddShard(int32 Count)
{
    (void)Count;
}

FMagicShardRuntimeStatus AMagicShardBaseCharacter::BuildRuntimeStatus() const
{
    FMagicShardRuntimeStatus Status;
    Status.Health = Health;
    Status.Mana = Mana;
    Status.Speed = GetVelocity().Size2D();
    Status.bGrounded = GetCharacterMovement() != nullptr && GetCharacterMovement()->IsMovingOnGround();
    Status.ActionState = ActionState;
    return Status;
}

float AMagicShardBaseCharacter::GetHealth() const
{
    return Health;
}

float AMagicShardBaseCharacter::GetMana() const
{
    return Mana;
}

EMagicShardActionState AMagicShardBaseCharacter::GetActionState() const
{
    return ActionState;
}

void AMagicShardBaseCharacter::UpdateMovementSpeed()
{
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp == nullptr)
    {
        return;
    }

    MoveComp->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;
}

void AMagicShardBaseCharacter::UpdateActionState(float DeltaSeconds)
{
    (void)DeltaSeconds;

    const float HorizontalSpeed = GetVelocity().Size2D();

    if (bSprinting)
    {
        // 冲刺中保持 Run，速度瞬时低于阈值时不切换
        // 防止地形碰撞导致动画闪烁（Idle→Run→Idle→Run）
        ActionState = EMagicShardActionState::Run;
    }
    else if (HorizontalSpeed < 5.0f)
    {
        ActionState = EMagicShardActionState::Idle;
    }
    else
    {
        ActionState = EMagicShardActionState::Walk;
    }
}

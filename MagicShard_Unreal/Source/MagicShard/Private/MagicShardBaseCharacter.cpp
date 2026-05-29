#include "MagicShardBaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

AMagicShardBaseCharacter::AMagicShardBaseCharacter()
    : MaxHealth(120.0f),
      Health(120.0f),
      MaxMana(80.0f),
      Mana(80.0f),
      WalkSpeed(450.0f),
      SprintSpeed(750.0f),
      BlockSpeed(180.0f),
      ActionState(EMagicShardActionState::Idle),
      bBlocking(false),
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

void AMagicShardBaseCharacter::StartPrimaryAction()
{
    ActionState = EMagicShardActionState::Attack;
}

void AMagicShardBaseCharacter::StopPrimaryAction()
{
    if (ActionState == EMagicShardActionState::Attack)
    {
        ActionState = EMagicShardActionState::Idle;
    }
}

void AMagicShardBaseCharacter::StartBlock()
{
    bBlocking = true;
    ActionState = EMagicShardActionState::Block;
    UpdateMovementSpeed();
}

void AMagicShardBaseCharacter::StopBlock()
{
    bBlocking = false;
    UpdateMovementSpeed();
}

void AMagicShardBaseCharacter::AddShard(int32 Count)
{
    // 基类不保存碎片数量，派生类可以重写。
    (void)Count;
}

void AMagicShardBaseCharacter::ReceiveDamage(float DamageAmount)
{
    if (DamageAmount <= 0.0f)
    {
        return;
    }

    const float FinalDamage = bBlocking ? DamageAmount * 0.35f : DamageAmount;
    Health = FMath::Clamp(Health - FinalDamage, 0.0f, MaxHealth);
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

    if (bBlocking)
    {
        MoveComp->MaxWalkSpeed = BlockSpeed;
    }
    else if (bSprinting)
    {
        MoveComp->MaxWalkSpeed = SprintSpeed;
    }
    else
    {
        MoveComp->MaxWalkSpeed = WalkSpeed;
    }
}

void AMagicShardBaseCharacter::UpdateActionState(float DeltaSeconds)
{
    (void)DeltaSeconds;

    if (bBlocking || ActionState == EMagicShardActionState::Attack)
    {
        return;
    }

    const bool bGrounded = GetCharacterMovement() != nullptr && GetCharacterMovement()->IsMovingOnGround();
    if (!bGrounded)
    {
        ActionState = EMagicShardActionState::Jump;
        return;
    }

    const float HorizontalSpeed = GetVelocity().Size2D();
    if (HorizontalSpeed < 5.0f)
    {
        ActionState = EMagicShardActionState::Idle;
    }
    else if (bSprinting)
    {
        ActionState = EMagicShardActionState::Run;
    }
    else
    {
        ActionState = EMagicShardActionState::Walk;
    }
}

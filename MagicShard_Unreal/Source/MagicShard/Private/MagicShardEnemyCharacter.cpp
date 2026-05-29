#include "MagicShardEnemyCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

AMagicShardEnemyCharacter::AMagicShardEnemyCharacter()
    : PatrolRadius(450.0f),
      DetectionRadius(800.0f),
      AttackRange(150.0f),
      PatrolTurnSpeed(0.8f),
      SpawnLocation(FVector::ZeroVector),
      PatrolAngle(0.0f)
{
    WalkSpeed = 220.0f;
    SprintSpeed = 420.0f;
    MaxHealth = 80.0f;
    Health = MaxHealth;
}

void AMagicShardEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();
    SpawnLocation = GetActorLocation();
}

void AMagicShardEnemyCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Patrol(DeltaSeconds);
}

void AMagicShardEnemyCharacter::StartPrimaryAction()
{
    Super::StartPrimaryAction();
    // 未来可以在这里连接伤害判定、蒙太奇或音效。
}

void AMagicShardEnemyCharacter::UpdateActionState(float DeltaSeconds)
{
    Super::UpdateActionState(DeltaSeconds);

    if (ActionState == EMagicShardActionState::Idle)
    {
        ActionState = EMagicShardActionState::Walk;
    }
}

void AMagicShardEnemyCharacter::Patrol(float DeltaSeconds)
{
    if (Controller == nullptr || PatrolRadius <= 1.0f)
    {
        return;
    }

    PatrolAngle += DeltaSeconds * PatrolTurnSpeed;
    const FVector TargetOffset(FMath::Cos(PatrolAngle) * PatrolRadius, FMath::Sin(PatrolAngle) * PatrolRadius, 0.0f);
    const FVector DesiredDirection = (SpawnLocation + TargetOffset - GetActorLocation()).GetSafeNormal2D();
    AddMovementInput(DesiredDirection, 0.35f);
}

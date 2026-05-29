#pragma once

#include "CoreMinimal.h"
#include "MagicShardBaseCharacter.h"
#include "MagicShardEnemyCharacter.generated.h"

/**
 * 敌人角色类。
 * 它和玩家一样继承自 AMagicShardBaseCharacter，
 * 用多态重写攻击行为和状态更新，便于后续扩展 Boss。
 */
UCLASS()
class MAGICSHARD_API AMagicShardEnemyCharacter : public AMagicShardBaseCharacter
{
    GENERATED_BODY()

public:
    AMagicShardEnemyCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual void StartPrimaryAction() override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|AI")
    float PatrolRadius;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|AI")
    float DetectionRadius;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|AI")
    float AttackRange;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|AI")
    float PatrolTurnSpeed;

    FVector SpawnLocation;
    float PatrolAngle;

    virtual void BeginPlay() override;
    virtual void UpdateActionState(float DeltaSeconds) override;
    void Patrol(float DeltaSeconds);
};

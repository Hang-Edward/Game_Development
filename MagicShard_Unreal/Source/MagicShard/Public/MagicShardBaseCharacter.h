#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MagicShardTypes.h"
#include "MagicShardBaseCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * 角色 Actor 的基类。
 *
 * 这里继承 Unreal 的 ACharacter，直接使用 CharacterMovementComponent 处理：
 * - 地面碰撞
 * - 上坡下坡
 * - 跳跃
 * - 空中水平速度
 *
 * 这样可以避免 Unity 版本中手写地面检测、视觉抬高、动画 root 偏移带来的问题。
 */
UCLASS(Abstract)
class MAGICSHARD_API AMagicShardBaseCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AMagicShardBaseCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

    virtual void StartPrimaryAction();
    virtual void StopPrimaryAction();
    virtual void StartBlock();
    virtual void StopBlock();
    virtual void AddShard(int32 Count);
    virtual void ReceiveDamage(float DamageAmount);
    virtual FMagicShardRuntimeStatus BuildRuntimeStatus() const;

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Character")
    float GetHealth() const;

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Character")
    float GetMana() const;

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Character")
    EMagicShardActionState GetActionState() const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Stats")
    float MaxHealth;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Stats")
    float Health;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Stats")
    float MaxMana;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Stats")
    float Mana;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Movement")
    float WalkSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Movement")
    float SprintSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Movement")
    float BlockSpeed;

    UPROPERTY(BlueprintReadOnly, Category = "MagicShard|State")
    EMagicShardActionState ActionState;

    UPROPERTY(BlueprintReadOnly, Category = "MagicShard|State")
    bool bBlocking;

    UPROPERTY(BlueprintReadOnly, Category = "MagicShard|State")
    bool bSprinting;

    virtual void UpdateMovementSpeed();
    virtual void UpdateActionState(float DeltaSeconds);
};

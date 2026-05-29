#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MagicShardCombatComponent.generated.h"

class AMagicShardBaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMagicShardDamageEvent, AActor*, Target, float, Damage);

/**
 * 战斗组件封装攻击冷却、攻击距离和伤害计算。
 * 角色类不直接写复杂战斗规则，体现封装和职责分离。
 */
UCLASS(ClassGroup = (MagicShard), meta = (BlueprintSpawnableComponent))
class MAGICSHARD_API UMagicShardCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMagicShardCombatComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Combat")
    bool TryMeleeAttack();

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Combat")
    bool CanAttack() const;

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Combat")
    void SetAttackDamage(float NewDamage);

    UPROPERTY(BlueprintAssignable, Category = "MagicShard|Combat")
    FMagicShardDamageEvent OnDamageApplied;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Combat")
    float AttackDamage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Combat")
    float AttackRange;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Combat")
    float AttackCooldown;

    float CooldownRemaining;

    AMagicShardBaseCharacter* OwnerCharacter;

    bool FindAttackTarget(FHitResult& OutHit) const;
};

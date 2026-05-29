#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MagicShardTypes.h"
#include "MagicShardEntity.generated.h"

/**
 * 所有游戏实体的抽象基类。
 *
 * 课程要求点：
 * - 类：UMagicShardEntity 本身是一个类。
 * - 对象：运行时可以创建 UObject 实例。
 * - 封装：Name、Id、bActive 都通过方法访问。
 * - 多态：GetDisplayName、TickEntity、DescribeEntity 是 virtual。
 */
UCLASS(Abstract, BlueprintType)
class MAGICSHARD_API UMagicShardEntity : public UObject
{
    GENERATED_BODY()

public:
    UMagicShardEntity();

    virtual FString GetDisplayName() const;
    virtual FString DescribeEntity() const;
    virtual void TickEntity(float DeltaSeconds);

    int32 GetEntityId() const;
    void SetEntityId(int32 NewId);

    bool IsActive() const;
    void SetActive(bool bNewActive);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entity")
    int32 EntityId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entity")
    FString EntityName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entity")
    bool bActive;
};

/**
 * 带生命值和法力值的实体。
 * 继承层次：UMagicShardEntity -> UMagicShardLivingEntity。
 */
UCLASS(Abstract, BlueprintType)
class MAGICSHARD_API UMagicShardLivingEntity : public UMagicShardEntity
{
    GENERATED_BODY()

public:
    UMagicShardLivingEntity();

    virtual FString DescribeEntity() const override;
    virtual void TickEntity(float DeltaSeconds) override;

    virtual void ApplyDamage(float Amount);
    virtual void RestoreHealth(float Amount);
    virtual bool CanAct() const;

    float GetHealth() const;
    float GetMana() const;
    void SetHealth(float NewHealth);
    void SetMana(float NewMana);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Living")
    float Health;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Living")
    float Mana;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Living")
    float MaxHealth;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Living")
    float MaxMana;
};

/**
 * 玩家英雄实体。
 * 继承层次：UMagicShardEntity -> UMagicShardLivingEntity -> UMagicShardHeroEntity，
 * 自定义继承深度达到 3 层，满足“继承层次不少于 2 层”的要求。
 */
UCLASS(BlueprintType)
class MAGICSHARD_API UMagicShardHeroEntity : public UMagicShardLivingEntity
{
    GENERATED_BODY()

public:
    UMagicShardHeroEntity();

    virtual FString GetDisplayName() const override;
    virtual FString DescribeEntity() const override;
    virtual void TickEntity(float DeltaSeconds) override;

    void AddShard(int32 Count);
    bool SpendMana(float Cost);
    int32 GetShardCount() const;

    FMagicShardPlayerRecord ToRecord(int32 SlotIndex, const FVector& Position) const;
    void FromRecord(const FMagicShardPlayerRecord& Record);

private:
    UPROPERTY(EditAnywhere, Category = "Hero")
    int32 CollectedShards;

    UPROPERTY(EditAnywhere, Category = "Hero")
    int32 Level;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagicShardPrototypeWorldBuilder.generated.h"

class AMagicShardShardPickup;

/**
 * 原型场景生成器。
 * 在没有手动搭建关卡之前，它可以生成地面、坡道、碎片拾取物和光照，
 * 方便用命令行打开工程后立刻测试移动、跳跃、碰撞和 UI。
 */
UCLASS()
class MAGICSHARD_API AMagicShardPrototypeWorldBuilder : public AActor
{
    GENERATED_BODY()

public:
    AMagicShardPrototypeWorldBuilder();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = "MagicShard|Prototype")
    int32 PickupCount;

    UPROPERTY(EditAnywhere, Category = "MagicShard|Prototype")
    float PickupRadius;

    UPROPERTY(EditAnywhere, Category = "MagicShard|Prototype")
    TSubclassOf<AMagicShardShardPickup> PickupClass;

    void SpawnPrototypeFloor();
    void SpawnPrototypePickups();
    void SpawnPrototypeLight();
    AActor* SpawnCube(const FVector& Location, const FVector& Scale, const FName& Name);
};

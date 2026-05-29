#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagicShardShardPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * 魔法碎片拾取物。
 * 玩家碰到它后增加碎片数量，并销毁自身。
 */
UCLASS()
class MAGICSHARD_API AMagicShardShardPickup : public AActor
{
    GENERATED_BODY()

public:
    AMagicShardShardPickup();

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MagicShard|Pickup")
    USphereComponent* CollisionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MagicShard|Pickup")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Pickup")
    int32 ShardValue;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Pickup")
    float BobHeight;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Pickup")
    float BobSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Pickup")
    float RotationSpeed;

    FVector StartLocation;
    float LifeTime;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

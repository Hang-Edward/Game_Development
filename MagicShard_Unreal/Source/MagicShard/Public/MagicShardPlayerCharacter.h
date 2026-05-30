#pragma once

#include "CoreMinimal.h"
#include "MagicShardBaseCharacter.h"
#include "MagicShardPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UAnimationAsset;
class UMagicShardHeroEntity;

/**
 * 玩家角色类。
 * 它继承 AMagicShardBaseCharacter，形成：
 * ACharacter -> AMagicShardBaseCharacter -> AMagicShardPlayerCharacter。
 */
UCLASS()
class MAGICSHARD_API AMagicShardPlayerCharacter : public AMagicShardBaseCharacter
{
    GENERATED_BODY()

public:
    AMagicShardPlayerCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void AddShard(int32 Count) override;

    void MoveByInput(const FVector2D& MoveValue);
    void LookByInput(const FVector2D& LookValue);
    void ZoomByInput(float WheelValue);
    void BeginSprint();
    void EndSprint();
    void RequestJump();
    void StopJumpRequest();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MagicShard|Camera")
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MagicShard|Camera")
    UCameraComponent* FollowCamera;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Camera")
    float CameraZoomStep;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Camera")
    float MinCameraDistance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Camera")
    float MaxCameraDistance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Camera")
    float CameraZoomSmoothSpeed;

    UPROPERTY()
    UMagicShardHeroEntity* HeroEntity;

    UPROPERTY(Transient)
    UAnimationAsset* IdleAnimation;

    UPROPERTY(Transient)
    UAnimationAsset* WalkAnimation;

    UPROPERTY(Transient)
    UAnimationAsset* RunAnimation;

    float DesiredCameraDistance;
    EMagicShardActionState LastVisualActionState;

    virtual void ConfigureImportedVisuals();
    virtual void UpdateCameraZoom(float DeltaSeconds);
    virtual void UpdateVisualAnimation();
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MagicShardPlayerController.generated.h"

class AMagicShardPlayerCharacter;

UCLASS()
class MAGICSHARD_API AMagicShardPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMagicShardPlayerController();

    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;

protected:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void Zoom(float Value);
    void StartSprint();
    void StopSprint();
    void StartJump();
    void StopJump();

private:
    FVector2D CachedMoveInput;

    AMagicShardPlayerCharacter* GetMagicShardCharacter() const;
};

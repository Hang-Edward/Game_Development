#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MagicShardPlayerController.generated.h"

class AMagicShardPlayerCharacter;

/**
 * 玩家输入控制器。
 * 课程视角下它展示了对象之间的协作：Controller 读取输入，
 * Character 负责移动、跳跃、格挡和镜头缩放。
 */
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
    void StartAttack();
    void StartBlock();
    void StopBlock();
    void SaveSlotOne();
    void LoadSlotOne();
    void UpdateSlotOne();

private:
    FVector2D CachedMoveInput;

    AMagicShardPlayerCharacter* GetMagicShardCharacter() const;
};

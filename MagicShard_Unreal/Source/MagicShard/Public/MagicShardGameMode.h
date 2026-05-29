#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MagicShardGameMode.generated.h"

/**
 * 游戏模式类，集中指定默认 Pawn、Controller 和 HUD。
 */
UCLASS()
class MAGICSHARD_API AMagicShardGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMagicShardGameMode();

    virtual void BeginPlay() override;
};

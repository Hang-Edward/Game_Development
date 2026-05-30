#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MagicShardGameMode.generated.h"

UCLASS()
class MAGICSHARD_API AMagicShardGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMagicShardGameMode();

    virtual void BeginPlay() override;

private:
    void ApplyMapTextures();
};

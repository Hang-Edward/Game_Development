#include "MagicShardGameMode.h"

#include "MagicShardHUD.h"
#include "MagicShardPlayerCharacter.h"
#include "MagicShardPlayerController.h"

AMagicShardGameMode::AMagicShardGameMode()
{
    DefaultPawnClass = AMagicShardPlayerCharacter::StaticClass();
    PlayerControllerClass = AMagicShardPlayerController::StaticClass();
    HUDClass = AMagicShardHUD::StaticClass();
}

void AMagicShardGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Display, TEXT("[MagicShardSmoke] GameMode BeginPlay: pawn=%s controller=%s hud=%s"),
        *GetNameSafe(DefaultPawnClass),
        *GetNameSafe(PlayerControllerClass),
        *GetNameSafe(HUDClass));
}

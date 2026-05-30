#include "MagicShardHUD.h"

#include "Engine/Canvas.h"
#include "MagicShardBaseCharacter.h"
#include "MagicShardHUDWidget.h"

void AMagicShardHUD::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Display, TEXT("[MagicShardSmoke] HUD BeginPlay: owner=%s widgetClass=%s"),
        *GetNameSafe(GetOwningPawn()),
        *GetNameSafe(HUDWidgetClass));

    if (HUDWidgetClass != nullptr)
    {
        HUDWidget = CreateWidget<UMagicShardHUDWidget>(GetWorld(), HUDWidgetClass);
        if (HUDWidget != nullptr)
        {
            HUDWidget->AddToViewport();
        }
    }
}

void AMagicShardHUD::DrawHUD()
{
    Super::DrawHUD();

    const FString Text = BuildFallbackHudText();
    DrawText(Text, FLinearColor::White, 24.0f, 24.0f, nullptr, 1.1f);
}

FString AMagicShardHUD::BuildFallbackHudText() const
{
    const APawn* Pawn = GetOwningPawn();
    const AMagicShardBaseCharacter* Character = Cast<AMagicShardBaseCharacter>(Pawn);
    if (Character == nullptr)
    {
        return TEXT("MagicShard Unreal C++");
    }

    const FMagicShardRuntimeStatus Status = Character->BuildRuntimeStatus();
    const FString StateName = StaticEnum<EMagicShardActionState>()->GetNameStringByValue(static_cast<int64>(Status.ActionState));
    return FString::Printf(
        TEXT("[WASD] Move  [Shift] Sprint  [Space] Jump  [Wheel] Zoom\nHP %.0f  MP %.0f  Speed %.0f  State %s"),
        Status.Health,
        Status.Mana,
        Status.Speed,
        *StateName);
}

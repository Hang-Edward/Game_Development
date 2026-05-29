#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MagicShardHUD.generated.h"

class UMagicShardHUDWidget;

/**
 * HUD 类负责把角色状态显示到屏幕上。
 * 这满足课程的 UI 要求，并且保留 C++ 实现入口。
 */
UCLASS()
class MAGICSHARD_API AMagicShardHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void DrawHUD() override;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "MagicShard|UI")
    TSubclassOf<UMagicShardHUDWidget> HUDWidgetClass;

    UPROPERTY()
    UMagicShardHUDWidget* HUDWidget;

    FString BuildFallbackHudText() const;
};

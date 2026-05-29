#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MagicShardTypes.h"
#include "MagicShardHUDWidget.generated.h"

/**
 * C++ UI 基类。
 * 可以在 Unreal 编辑器中创建蓝图 Widget 继承它，
 * 也可以直接由 HUD 使用状态字符串展示。
 */
UCLASS()
class MAGICSHARD_API UMagicShardHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "MagicShard|UI")
    void SetRuntimeStatus(const FMagicShardRuntimeStatus& NewStatus);

    UFUNCTION(BlueprintPure, Category = "MagicShard|UI")
    FMagicShardRuntimeStatus GetRuntimeStatus() const;

    UFUNCTION(BlueprintPure, Category = "MagicShard|UI")
    FText BuildStatusText() const;

private:
    UPROPERTY(BlueprintReadOnly, Category = "MagicShard|UI", meta = (AllowPrivateAccess = "true"))
    FMagicShardRuntimeStatus RuntimeStatus;
};

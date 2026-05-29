#include "MagicShardHUDWidget.h"

void UMagicShardHUDWidget::SetRuntimeStatus(const FMagicShardRuntimeStatus& NewStatus)
{
    RuntimeStatus = NewStatus;
}

FMagicShardRuntimeStatus UMagicShardHUDWidget::GetRuntimeStatus() const
{
    return RuntimeStatus;
}

FText UMagicShardHUDWidget::BuildStatusText() const
{
    const FString StateName = StaticEnum<EMagicShardActionState>()->GetNameStringByValue(static_cast<int64>(RuntimeStatus.ActionState));
    return FText::FromString(FString::Printf(
        TEXT("HP %.0f | MP %.0f | Speed %.0f | %s"),
        RuntimeStatus.Health,
        RuntimeStatus.Mana,
        RuntimeStatus.Speed,
        *StateName));
}

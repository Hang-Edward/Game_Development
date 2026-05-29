#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MagicShardTypes.h"
#include "MagicShardSaveSubsystem.generated.h"

/**
 * Unreal 游戏实例级存档子系统。
 * 它把 UI/角色代码和底层随机文件处理隔离开，
 * 对外提供更易用的 Save/Load/Update 接口。
 */
UCLASS()
class MAGICSHARD_API UMagicShardSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Save")
    bool SavePlayerRecord(int32 SlotIndex, const FMagicShardPlayerRecord& Record);

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Save")
    bool LoadPlayerRecord(int32 SlotIndex, FMagicShardPlayerRecord& OutRecord) const;

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Save")
    bool UpdatePlayerRecord(int32 SlotIndex, const FMagicShardPlayerRecord& Record);

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Save")
    bool ClearPlayerRecord(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Save")
    FString GetSaveFilePath() const;

private:
    FString SaveFilePath;
    int32 SlotCount = 8;
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MagicShardInventoryComponent.generated.h"

/**
 * 背包中的一个物品堆叠。
 * 使用 USTRUCT 便于 UI 读取，也便于以后存档。
 */
USTRUCT(BlueprintType)
struct FMagicShardItemStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 Count = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 MaxStack = 99;

    bool IsValidStack() const
    {
        return !ItemId.IsNone() && Count > 0;
    }
};

/**
 * 通用背包组件。
 *
 * 这个类用于展示组合关系：角色 Actor 拥有 Component 对象，
 * Component 封装背包数组并通过方法控制增删查改。
 */
UCLASS(ClassGroup = (MagicShard), meta = (BlueprintSpawnableComponent))
class MAGICSHARD_API UMagicShardInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMagicShardInventoryComponent();

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Inventory")
    bool AddItem(FName ItemId, const FText& DisplayName, int32 Count, int32 MaxStack = 99);

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Inventory")
    bool RemoveItem(FName ItemId, int32 Count);

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Inventory")
    bool HasItem(FName ItemId, int32 Count = 1) const;

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Inventory")
    int32 GetItemCount(FName ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Inventory")
    void ClearInventory();

    UFUNCTION(BlueprintCallable, Category = "MagicShard|Inventory")
    TArray<FMagicShardItemStack> GetItems() const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Inventory")
    int32 Capacity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MagicShard|Inventory")
    TArray<FMagicShardItemStack> Items;

    int32 FindItemIndex(FName ItemId) const;
    bool CanCreateNewStack() const;
};

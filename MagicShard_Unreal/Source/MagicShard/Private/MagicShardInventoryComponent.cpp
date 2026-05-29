#include "MagicShardInventoryComponent.h"

UMagicShardInventoryComponent::UMagicShardInventoryComponent()
    : Capacity(24)
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UMagicShardInventoryComponent::AddItem(FName ItemId, const FText& DisplayName, int32 Count, int32 MaxStack)
{
    if (ItemId.IsNone() || Count <= 0 || MaxStack <= 0)
    {
        return false;
    }

    int32 Remaining = Count;
    while (Remaining > 0)
    {
        const int32 ExistingIndex = FindItemIndex(ItemId);
        if (ExistingIndex != INDEX_NONE && Items[ExistingIndex].Count < Items[ExistingIndex].MaxStack)
        {
            const int32 Space = Items[ExistingIndex].MaxStack - Items[ExistingIndex].Count;
            const int32 Added = FMath::Min(Space, Remaining);
            Items[ExistingIndex].Count += Added;
            Remaining -= Added;
            continue;
        }

        if (!CanCreateNewStack())
        {
            return Remaining != Count;
        }

        FMagicShardItemStack NewStack;
        NewStack.ItemId = ItemId;
        NewStack.DisplayName = DisplayName;
        NewStack.Count = FMath::Min(MaxStack, Remaining);
        NewStack.MaxStack = MaxStack;
        Items.Add(NewStack);
        Remaining -= NewStack.Count;
    }

    return true;
}

bool UMagicShardInventoryComponent::RemoveItem(FName ItemId, int32 Count)
{
    if (ItemId.IsNone() || Count <= 0 || GetItemCount(ItemId) < Count)
    {
        return false;
    }

    int32 Remaining = Count;
    for (int32 Index = Items.Num() - 1; Index >= 0 && Remaining > 0; --Index)
    {
        if (Items[Index].ItemId != ItemId)
        {
            continue;
        }

        const int32 Removed = FMath::Min(Items[Index].Count, Remaining);
        Items[Index].Count -= Removed;
        Remaining -= Removed;

        if (Items[Index].Count <= 0)
        {
            Items.RemoveAt(Index);
        }
    }

    return Remaining == 0;
}

bool UMagicShardInventoryComponent::HasItem(FName ItemId, int32 Count) const
{
    return GetItemCount(ItemId) >= Count;
}

int32 UMagicShardInventoryComponent::GetItemCount(FName ItemId) const
{
    int32 Total = 0;
    for (const FMagicShardItemStack& Stack : Items)
    {
        if (Stack.ItemId == ItemId)
        {
            Total += Stack.Count;
        }
    }

    return Total;
}

void UMagicShardInventoryComponent::ClearInventory()
{
    Items.Reset();
}

TArray<FMagicShardItemStack> UMagicShardInventoryComponent::GetItems() const
{
    return Items;
}

int32 UMagicShardInventoryComponent::FindItemIndex(FName ItemId) const
{
    for (int32 Index = 0; Index < Items.Num(); ++Index)
    {
        if (Items[Index].ItemId == ItemId)
        {
            return Index;
        }
    }

    return INDEX_NONE;
}

bool UMagicShardInventoryComponent::CanCreateNewStack() const
{
    return Items.Num() < Capacity;
}

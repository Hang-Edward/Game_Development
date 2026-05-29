#include "MagicShardSaveSubsystem.h"

#include "RandomAccessSaveFile.h"
#include "Misc/Paths.h"

void UMagicShardSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    SaveFilePath = FPaths::ProjectSavedDir() / TEXT("MagicShard") / TEXT("PlayerRecords.dat");
    FRandomAccessSaveFile SaveFile(SaveFilePath);
    SaveFile.EnsureFileExists(SlotCount);

    UE_LOG(LogTemp, Display, TEXT("[MagicShardSmoke] SaveSubsystem initialized: path=%s slots=%d"),
        *SaveFilePath,
        SlotCount);
}

bool UMagicShardSaveSubsystem::SavePlayerRecord(int32 SlotIndex, const FMagicShardPlayerRecord& Record)
{
    FRandomAccessSaveFile SaveFile(SaveFilePath);
    return SaveFile.EnsureFileExists(SlotCount) && SaveFile.WriteRecord(SlotIndex, Record);
}

bool UMagicShardSaveSubsystem::LoadPlayerRecord(int32 SlotIndex, FMagicShardPlayerRecord& OutRecord) const
{
    FRandomAccessSaveFile SaveFile(SaveFilePath);
    return SaveFile.ReadRecord(SlotIndex, OutRecord);
}

bool UMagicShardSaveSubsystem::UpdatePlayerRecord(int32 SlotIndex, const FMagicShardPlayerRecord& Record)
{
    FRandomAccessSaveFile SaveFile(SaveFilePath);
    return SaveFile.EnsureFileExists(SlotCount) && SaveFile.UpdateRecord(SlotIndex, Record);
}

bool UMagicShardSaveSubsystem::ClearPlayerRecord(int32 SlotIndex)
{
    FRandomAccessSaveFile SaveFile(SaveFilePath);
    return SaveFile.EnsureFileExists(SlotCount) && SaveFile.ClearRecord(SlotIndex);
}

FString UMagicShardSaveSubsystem::GetSaveFilePath() const
{
    return SaveFilePath;
}

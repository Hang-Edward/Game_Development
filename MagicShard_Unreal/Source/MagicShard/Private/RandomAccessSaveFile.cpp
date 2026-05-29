#include "RandomAccessSaveFile.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

#include <algorithm>
#include <cstring>
#include <fstream>

FMagicShardDiskRecord::FMagicShardDiskRecord()
    : SlotIndex(-1),
      Level(1),
      Health(100.0f),
      Mana(50.0f),
      X(0.0f),
      Y(0.0f),
      Z(0.0f),
      CollectedShards(0)
{
    std::memset(PlayerName, 0, sizeof(PlayerName));
}

FRandomAccessSaveFile::FRandomAccessSaveFile(const FString& InFilePath)
    : FilePath(InFilePath)
{
}

bool FRandomAccessSaveFile::EnsureFileExists(int32 SlotCount)
{
    if (SlotCount <= 0)
    {
        return false;
    }

    const FString Directory = FPaths::GetPath(FilePath);
    if (!Directory.IsEmpty())
    {
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        if (!PlatformFile.DirectoryExists(*Directory))
        {
            PlatformFile.CreateDirectoryTree(*Directory);
        }
    }

    const std::string Path = ToStdPath();
    std::fstream File(Path, std::ios::binary | std::ios::in | std::ios::out);
    if (!File.is_open())
    {
        std::ofstream NewFile(Path, std::ios::binary | std::ios::out);
        if (!NewFile.is_open())
        {
            return false;
        }

        FMagicShardDiskRecord EmptyRecord;
        for (int32 Index = 0; Index < SlotCount; ++Index)
        {
            EmptyRecord.SlotIndex = Index;
            NewFile.write(reinterpret_cast<const char*>(&EmptyRecord), sizeof(FMagicShardDiskRecord));
        }

        NewFile.close();
        return true;
    }

    File.seekg(0, std::ios::end);
    const std::streamoff ExistingSize = File.tellg();
    const std::streamoff RequiredSize = static_cast<std::streamoff>(SlotCount) * sizeof(FMagicShardDiskRecord);
    if (ExistingSize >= RequiredSize)
    {
        File.close();
        return true;
    }

    File.seekp(0, std::ios::end);
    FMagicShardDiskRecord EmptyRecord;
    const int32 ExistingSlots = static_cast<int32>(ExistingSize / sizeof(FMagicShardDiskRecord));
    for (int32 Index = ExistingSlots; Index < SlotCount; ++Index)
    {
        EmptyRecord = FMagicShardDiskRecord();
        EmptyRecord.SlotIndex = Index;
        File.write(reinterpret_cast<const char*>(&EmptyRecord), sizeof(FMagicShardDiskRecord));
    }

    File.close();
    return true;
}

bool FRandomAccessSaveFile::WriteRecord(int32 SlotIndex, const FMagicShardPlayerRecord& Record)
{
    if (!IsSlotValid(SlotIndex))
    {
        return false;
    }

    const std::string Path = ToStdPath();
    std::fstream File(Path, std::ios::binary | std::ios::in | std::ios::out);
    if (!File.is_open())
    {
        return false;
    }

    const FMagicShardDiskRecord DiskRecord = ToDiskRecord(SlotIndex, Record);
    File.seekp(GetRecordOffset(SlotIndex), std::ios::beg);
    File.write(reinterpret_cast<const char*>(&DiskRecord), sizeof(FMagicShardDiskRecord));
    const bool bSuccess = File.good();
    File.close();
    return bSuccess;
}

bool FRandomAccessSaveFile::ReadRecord(int32 SlotIndex, FMagicShardPlayerRecord& OutRecord) const
{
    if (!IsSlotValid(SlotIndex))
    {
        return false;
    }

    const std::string Path = ToStdPath();
    std::ifstream File(Path, std::ios::binary | std::ios::in);
    if (!File.is_open())
    {
        return false;
    }

    FMagicShardDiskRecord DiskRecord;
    File.seekg(GetRecordOffset(SlotIndex), std::ios::beg);
    File.read(reinterpret_cast<char*>(&DiskRecord), sizeof(FMagicShardDiskRecord));
    const bool bSuccess = File.good() || File.gcount() == sizeof(FMagicShardDiskRecord);
    File.close();

    if (!bSuccess)
    {
        return false;
    }

    OutRecord = FromDiskRecord(DiskRecord);
    return true;
}

bool FRandomAccessSaveFile::UpdateRecord(int32 SlotIndex, const FMagicShardPlayerRecord& UpdatedRecord)
{
    FMagicShardPlayerRecord ExistingRecord;
    if (!ReadRecord(SlotIndex, ExistingRecord))
    {
        return false;
    }

    FMagicShardPlayerRecord MergedRecord = ExistingRecord;
    MergedRecord.SlotIndex = SlotIndex;
    MergedRecord.Level = UpdatedRecord.Level;
    MergedRecord.Health = UpdatedRecord.Health;
    MergedRecord.Mana = UpdatedRecord.Mana;
    MergedRecord.Position = UpdatedRecord.Position;
    MergedRecord.PlayerName = UpdatedRecord.PlayerName;
    MergedRecord.CollectedShards = UpdatedRecord.CollectedShards;
    return WriteRecord(SlotIndex, MergedRecord);
}

bool FRandomAccessSaveFile::ReadAllRecords(TArray<FMagicShardPlayerRecord>& OutRecords, int32 SlotCount) const
{
    OutRecords.Reset();
    if (SlotCount <= 0)
    {
        return false;
    }

    for (int32 Index = 0; Index < SlotCount; ++Index)
    {
        FMagicShardPlayerRecord Record;
        if (ReadRecord(Index, Record))
        {
            OutRecords.Add(Record);
        }
    }

    return OutRecords.Num() > 0;
}

bool FRandomAccessSaveFile::ClearRecord(int32 SlotIndex)
{
    if (!IsSlotValid(SlotIndex))
    {
        return false;
    }

    FMagicShardPlayerRecord EmptyRecord;
    EmptyRecord.SlotIndex = SlotIndex;
    EmptyRecord.PlayerName = TEXT("Empty");
    EmptyRecord.Level = 1;
    EmptyRecord.Health = 100.0f;
    EmptyRecord.Mana = 50.0f;
    EmptyRecord.Position = FVector::ZeroVector;
    EmptyRecord.CollectedShards = 0;
    return WriteRecord(SlotIndex, EmptyRecord);
}

FString FRandomAccessSaveFile::GetFilePath() const
{
    return FilePath;
}

int64 FRandomAccessSaveFile::GetRecordOffset(int32 SlotIndex) const
{
    return static_cast<int64>(SlotIndex) * static_cast<int64>(sizeof(FMagicShardDiskRecord));
}

bool FRandomAccessSaveFile::IsSlotValid(int32 SlotIndex) const
{
    return SlotIndex >= 0;
}

FMagicShardDiskRecord FRandomAccessSaveFile::ToDiskRecord(int32 SlotIndex, const FMagicShardPlayerRecord& Record) const
{
    FMagicShardDiskRecord DiskRecord;
    DiskRecord.SlotIndex = SlotIndex;
    DiskRecord.Level = Record.Level;
    DiskRecord.Health = Record.Health;
    DiskRecord.Mana = Record.Mana;
    DiskRecord.X = Record.Position.X;
    DiskRecord.Y = Record.Position.Y;
    DiskRecord.Z = Record.Position.Z;
    DiskRecord.CollectedShards = Record.CollectedShards;

    const std::string Name = TCHAR_TO_UTF8(*Record.PlayerName.Left(63));
    std::memset(DiskRecord.PlayerName, 0, sizeof(DiskRecord.PlayerName));
    std::memcpy(DiskRecord.PlayerName, Name.c_str(), std::min(Name.size(), sizeof(DiskRecord.PlayerName) - 1));
    return DiskRecord;
}

FMagicShardPlayerRecord FRandomAccessSaveFile::FromDiskRecord(const FMagicShardDiskRecord& DiskRecord) const
{
    FMagicShardPlayerRecord Record;
    Record.SlotIndex = DiskRecord.SlotIndex;
    Record.Level = DiskRecord.Level;
    Record.Health = DiskRecord.Health;
    Record.Mana = DiskRecord.Mana;
    Record.Position = FVector(DiskRecord.X, DiskRecord.Y, DiskRecord.Z);
    Record.PlayerName = FString(UTF8_TO_TCHAR(DiskRecord.PlayerName));
    Record.CollectedShards = DiskRecord.CollectedShards;
    return Record;
}

std::string FRandomAccessSaveFile::ToStdPath() const
{
    return std::string(TCHAR_TO_UTF8(*FilePath));
}

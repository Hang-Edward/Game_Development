#pragma once

#include "CoreMinimal.h"
#include "MagicShardTypes.h"

#include <string>

/**
 * 写入磁盘的定长记录。
 *
 * 注意：这是普通 C++ struct，不使用 UObject 反射。
 * 原因是随机文件处理需要固定长度记录，便于通过 seekg/seekp 定位。
 */
struct FMagicShardDiskRecord
{
    int32 SlotIndex;
    int32 Level;
    float Health;
    float Mana;
    float X;
    float Y;
    float Z;
    int32 CollectedShards;
    char PlayerName[64];

    FMagicShardDiskRecord();
};

/**
 * 课程要求的随机文件处理类。
 *
 * 它明确实现：
 * - writing：WriteRecord
 * - reading：ReadRecord
 * - updating：UpdateRecord
 *
 * 文件中每个槽位都是固定长度记录，因此可以使用 seekp/seekg 直接跳到
 * 第 N 个记录的位置，而不是顺序扫描整个文件。
 */
class MAGICSHARD_API FRandomAccessSaveFile
{
public:
    explicit FRandomAccessSaveFile(const FString& InFilePath);

    bool EnsureFileExists(int32 SlotCount);
    bool WriteRecord(int32 SlotIndex, const FMagicShardPlayerRecord& Record);
    bool ReadRecord(int32 SlotIndex, FMagicShardPlayerRecord& OutRecord) const;
    bool UpdateRecord(int32 SlotIndex, const FMagicShardPlayerRecord& UpdatedRecord);
    bool ReadAllRecords(TArray<FMagicShardPlayerRecord>& OutRecords, int32 SlotCount) const;
    bool ClearRecord(int32 SlotIndex);

    FString GetFilePath() const;
    int64 GetRecordOffset(int32 SlotIndex) const;

private:
    FString FilePath;

    bool IsSlotValid(int32 SlotIndex) const;
    FMagicShardDiskRecord ToDiskRecord(int32 SlotIndex, const FMagicShardPlayerRecord& Record) const;
    FMagicShardPlayerRecord FromDiskRecord(const FMagicShardDiskRecord& DiskRecord) const;
    std::string ToStdPath() const;
};

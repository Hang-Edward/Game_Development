#pragma once

#include "CoreMinimal.h"
#include "MagicShardTypes.generated.h"

/**
 * 角色当前的高层动作状态。
 * 这个枚举用于 UI、存档和角色控制之间传递清晰的语义，
 * 避免到处使用难读的字符串或裸整数。
 */
UENUM(BlueprintType)
enum class EMagicShardActionState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    Walk UMETA(DisplayName = "Walk"),
    Run UMETA(DisplayName = "Run")
};

/**
 * 随机文件中的玩家记录。
 * USTRUCT 负责给 Unreal UI/蓝图展示，真正的随机文件读写由
 * FRandomAccessSaveFile 使用标准 C++ fstream 完成。
 */
USTRUCT(BlueprintType)
struct FMagicShardPlayerRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Record")
    int32 SlotIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Record")
    int32 Level = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Record")
    float Health = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Record")
    float Mana = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Record")
    FVector Position = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Record")
    FString PlayerName = TEXT("Hero");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Record")
    int32 CollectedShards = 0;

    FString ToDebugString() const
    {
        return FString::Printf(
            TEXT("Slot=%d Name=%s Level=%d HP=%.1f MP=%.1f Shards=%d Pos=(%.1f, %.1f, %.1f)"),
            SlotIndex,
            *PlayerName,
            Level,
            Health,
            Mana,
            CollectedShards,
            Position.X,
            Position.Y,
            Position.Z);
    }
};

/**
 * 运行时角色状态快照。
 * PlayerController、HUD 和 SaveSubsystem 之间通过这个结构交换状态，
 * 可以减少类之间的直接依赖。
 */
USTRUCT(BlueprintType)
struct FMagicShardRuntimeStatus
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    float Health = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    float Mana = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    float Speed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    bool bGrounded = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    EMagicShardActionState ActionState = EMagicShardActionState::Idle;
};

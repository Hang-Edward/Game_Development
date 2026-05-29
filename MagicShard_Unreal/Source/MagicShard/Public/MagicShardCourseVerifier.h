#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MagicShardCourseVerifier.generated.h"

/**
 * 课程合规检查工具。
 * 这个类不参与核心玩法，但可以在 UI 或蓝图中调用，
 * 用来向老师展示项目如何满足 C++ 面向对象要求。
 */
UCLASS()
class MAGICSHARD_API UMagicShardCourseVerifier : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "MagicShard|Course")
    static TArray<FString> BuildRequirementReport();

    UFUNCTION(BlueprintPure, Category = "MagicShard|Course")
    static int32 GetCustomClassCount();

    UFUNCTION(BlueprintPure, Category = "MagicShard|Course")
    static bool HasRandomFileWriting();

    UFUNCTION(BlueprintPure, Category = "MagicShard|Course")
    static bool HasRandomFileReading();

    UFUNCTION(BlueprintPure, Category = "MagicShard|Course")
    static bool HasRandomFileUpdating();
};

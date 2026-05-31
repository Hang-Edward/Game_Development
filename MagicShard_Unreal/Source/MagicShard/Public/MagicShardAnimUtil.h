#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MagicShardAnimUtil.generated.h"

UCLASS()
class MAGICSHARD_API UMagicShardAnimUtil : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "MagicShard|Animation")
    static bool ExportRunAnimLoopFixData();
};

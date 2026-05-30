#include "MagicShardGameMode.h"

#include "EngineUtils.h"
#include "MagicShardHUD.h"
#include "MagicShardPlayerCharacter.h"
#include "MagicShardPlayerController.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"

AMagicShardGameMode::AMagicShardGameMode()
{
    DefaultPawnClass = AMagicShardPlayerCharacter::StaticClass();
    PlayerControllerClass = AMagicShardPlayerController::StaticClass();
    HUDClass = AMagicShardHUD::StaticClass();
}

void AMagicShardGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Display, TEXT("[MagicShardSmoke] GameMode BeginPlay: pawn=%s controller=%s hud=%s"),
        *GetNameSafe(DefaultPawnClass),
        *GetNameSafe(PlayerControllerClass),
        *GetNameSafe(HUDClass));

    ApplyMapTextures();
}

void AMagicShardGameMode::ApplyMapTextures()
{
    UTexture* Tex0 = LoadObject<UTexture>(nullptr, TEXT("/Game/Imported/Map/Textures/Image_0.Image_0"));
    UTexture* Tex1 = LoadObject<UTexture>(nullptr, TEXT("/Game/Imported/Map/Textures/Image_1.Image_1"));
    UMaterial* ParentMat = LoadObject<UMaterial>(nullptr, TEXT("/Game/Imported/Map/M_UnityTextureParent.M_UnityTextureParent"));

    if (ParentMat == nullptr || (Tex0 == nullptr && Tex1 == nullptr))
    {
        UE_LOG(LogTemp, Warning, TEXT("[MapTex] Skip fix: no textures or parent material found"));
        return;
    }

    TArray<FName> TexParamNames;
    const TArray<TObjectPtr<UMaterialExpression>>& Expressions = ParentMat->GetExpressionCollection().Expressions;
    for (const auto& ExprPtr : Expressions)
    {
        UMaterialExpression* Expr = ExprPtr.Get();
        if (Expr == nullptr) continue;
        UMaterialExpressionTextureSampleParameter* TexParam = Cast<UMaterialExpressionTextureSampleParameter>(Expr);
        if (TexParam != nullptr)
        {
            TexParamNames.AddUnique(TexParam->ParameterName);
        }
    }
    if (TexParamNames.Num() == 0)
    {
        TexParamNames.Add(FName(TEXT("Diffuse")));
    }

    UWorld* World = GetWorld();
    if (World == nullptr) return;

    for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
    {
        AStaticMeshActor* Actor = *It;
        UStaticMeshComponent* MeshComp = Actor->GetStaticMeshComponent();
        if (MeshComp == nullptr || MeshComp->GetStaticMesh() == nullptr) continue;

        if (!MeshComp->GetStaticMesh()->GetName().Contains(TEXT("map_01_forest"))) continue;

        int32 NumSlots = MeshComp->GetNumMaterials();
        UE_LOG(LogTemp, Display, TEXT("[MapTex] Found map mesh with %d material slots"), NumSlots);

        UTexture* Textures[] = { Tex0, Tex1 };

        for (int32 SlotIdx = 0; SlotIdx < NumSlots; SlotIdx++)
        {
            UTexture* Tex = (SlotIdx < 2) ? Textures[SlotIdx] : nullptr;

            UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ParentMat, this);
            if (MID == nullptr) continue;

            if (Tex != nullptr)
            {
                for (FName ParamName : TexParamNames)
                {
                    MID->SetTextureParameterValue(ParamName, Tex);
                }
                UE_LOG(LogTemp, Display, TEXT("[MapTex] Slot %d -> %s (param=%s)"),
                    SlotIdx, *Tex->GetName(), *TexParamNames[0].ToString());
            }

            MeshComp->SetMaterial(SlotIdx, MID);
        }

        UE_LOG(LogTemp, Display, TEXT("[MapTex] Map textures applied successfully"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[MapTex] Map mesh not found in level"));
}

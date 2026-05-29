#include "MagicShardPrototypeWorldBuilder.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "MagicShardShardPickup.h"

AMagicShardPrototypeWorldBuilder::AMagicShardPrototypeWorldBuilder()
    : PickupCount(8),
      PickupRadius(600.0f)
{
    PrimaryActorTick.bCanEverTick = false;
}

void AMagicShardPrototypeWorldBuilder::BeginPlay()
{
    Super::BeginPlay();

    SpawnPrototypeFloor();
    SpawnPrototypeLight();
    SpawnPrototypePickups();
}

void AMagicShardPrototypeWorldBuilder::SpawnPrototypeFloor()
{
    SpawnCube(FVector(0.0f, 0.0f, -55.0f), FVector(18.0f, 18.0f, 1.0f), TEXT("PrototypeFloor"));
    AActor* RampA = SpawnCube(FVector(420.0f, 0.0f, 20.0f), FVector(5.0f, 2.5f, 0.35f), TEXT("PrototypeRampA"));
    if (RampA != nullptr)
    {
        RampA->SetActorRotation(FRotator(0.0f, 0.0f, 12.0f));
    }

    AActor* RampB = SpawnCube(FVector(-420.0f, 220.0f, 35.0f), FVector(4.0f, 2.0f, 0.35f), TEXT("PrototypeRampB"));
    if (RampB != nullptr)
    {
        RampB->SetActorRotation(FRotator(0.0f, 25.0f, -10.0f));
    }
}

void AMagicShardPrototypeWorldBuilder::SpawnPrototypePickups()
{
    UWorld* World = GetWorld();
    if (World == nullptr || PickupClass == nullptr)
    {
        return;
    }

    for (int32 Index = 0; Index < PickupCount; ++Index)
    {
        const float Angle = static_cast<float>(Index) / FMath::Max(1, PickupCount) * PI * 2.0f;
        const FVector Location(FMath::Cos(Angle) * PickupRadius, FMath::Sin(Angle) * PickupRadius, 90.0f);
        World->SpawnActor<AMagicShardShardPickup>(PickupClass, Location, FRotator::ZeroRotator);
    }
}

void AMagicShardPrototypeWorldBuilder::SpawnPrototypeLight()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(-300.0f, -400.0f, 600.0f), FRotator(-45.0f, -35.0f, 0.0f));
    if (Light != nullptr)
    {
        Light->SetActorScale3D(FVector(1.0f));
    }
}

AActor* AMagicShardPrototypeWorldBuilder::SpawnCube(const FVector& Location, const FVector& Scale, const FName& Name)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return nullptr;
    }

    AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, FRotator::ZeroRotator);
    if (MeshActor == nullptr)
    {
        return nullptr;
    }

    (void)Name;
    MeshActor->SetActorScale3D(Scale);
    UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
    if (MeshComp != nullptr)
    {
        UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (CubeMesh != nullptr)
        {
            MeshComp->SetStaticMesh(CubeMesh);
        }

        MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
    }

    return MeshActor;
}

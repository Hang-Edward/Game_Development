#include "MagicShardShardPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MagicShardPlayerCharacter.h"

AMagicShardShardPickup::AMagicShardShardPickup()
    : ShardValue(1),
      BobHeight(18.0f),
      BobSpeed(2.5f),
      RotationSpeed(90.0f),
      StartLocation(FVector::ZeroVector),
      LifeTime(0.0f)
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;
    CollisionSphere->InitSphereRadius(55.0f);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMagicShardShardPickup::BeginPlay()
{
    Super::BeginPlay();

    StartLocation = GetActorLocation();
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMagicShardShardPickup::OnOverlapBegin);
}

void AMagicShardShardPickup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    LifeTime += DeltaSeconds;
    const float BobOffset = FMath::Sin(LifeTime * BobSpeed) * BobHeight;
    SetActorLocation(StartLocation + FVector(0.0f, 0.0f, BobOffset));
    AddActorWorldRotation(FRotator(0.0f, RotationSpeed * DeltaSeconds, 0.0f));
}

void AMagicShardShardPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    (void)OverlappedComponent;
    (void)OtherComp;
    (void)OtherBodyIndex;
    (void)bFromSweep;
    (void)SweepResult;

    AMagicShardPlayerCharacter* Player = Cast<AMagicShardPlayerCharacter>(OtherActor);
    if (Player == nullptr)
    {
        return;
    }

    Player->AddShard(ShardValue);
    Destroy();
}

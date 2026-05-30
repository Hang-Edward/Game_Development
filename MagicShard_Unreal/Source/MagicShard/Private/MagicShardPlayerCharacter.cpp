#include "MagicShardPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimationAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "MagicShardEntity.h"
#include "Components/SkeletalMeshComponent.h"

AMagicShardPlayerCharacter::AMagicShardPlayerCharacter()
    : CameraZoomStep(60.0f),
      MinCameraDistance(250.0f),
      MaxCameraDistance(900.0f),
      CameraZoomSmoothSpeed(12.0f),
      HeroEntity(nullptr),
      IdleAnimation(nullptr),
      WalkAnimation(nullptr),
      RunAnimation(nullptr),
      DesiredCameraDistance(550.0f),
      LastVisualActionState(EMagicShardActionState::Idle)
{
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = DesiredCameraDistance;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 14.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 80.0f);

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    ConfigureImportedVisuals();
}

void AMagicShardPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    HeroEntity = NewObject<UMagicShardHeroEntity>(this);
    if (HeroEntity != nullptr)
    {
        HeroEntity->SetEntityId(1);
    }

    UE_LOG(LogTemp, Display, TEXT("[MagicShardSmoke] PlayerCharacter BeginPlay: location=%s controller=%s"),
        *GetActorLocation().ToCompactString(),
        *GetNameSafe(GetController()));
}

void AMagicShardPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (HeroEntity != nullptr)
    {
        HeroEntity->TickEntity(DeltaSeconds);
        Health = HeroEntity->GetHealth();
        Mana = HeroEntity->GetMana();
    }

    UpdateCameraZoom(DeltaSeconds);
    UpdateVisualAnimation();
}

void AMagicShardPlayerCharacter::AddShard(int32 Count)
{
    if (HeroEntity != nullptr)
    {
        HeroEntity->AddShard(Count);
    }
}

void AMagicShardPlayerCharacter::MoveByInput(const FVector2D& MoveValue)
{
    if (Controller == nullptr || MoveValue.IsNearlyZero())
    {
        return;
    }

    const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDirection, MoveValue.Y);
    AddMovementInput(RightDirection, MoveValue.X);
}

void AMagicShardPlayerCharacter::LookByInput(const FVector2D& LookValue)
{
    AddControllerYawInput(LookValue.X);
    AddControllerPitchInput(LookValue.Y);
}

void AMagicShardPlayerCharacter::ZoomByInput(float WheelValue)
{
    if (FMath::IsNearlyZero(WheelValue))
    {
        return;
    }

    DesiredCameraDistance = FMath::Clamp(DesiredCameraDistance - WheelValue * CameraZoomStep, MinCameraDistance, MaxCameraDistance);
}

void AMagicShardPlayerCharacter::BeginSprint()
{
    bSprinting = true;
    UpdateMovementSpeed();
}

void AMagicShardPlayerCharacter::EndSprint()
{
    bSprinting = false;
    UpdateMovementSpeed();
}

void AMagicShardPlayerCharacter::RequestJump()
{
    Jump();
}

void AMagicShardPlayerCharacter::StopJumpRequest()
{
    StopJumping();
}

void AMagicShardPlayerCharacter::UpdateCameraZoom(float DeltaSeconds)
{
    if (CameraBoom == nullptr)
    {
        return;
    }

    DesiredCameraDistance = FMath::Clamp(DesiredCameraDistance, MinCameraDistance, MaxCameraDistance);
    CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DesiredCameraDistance, DeltaSeconds, CameraZoomSmoothSpeed);
}

void AMagicShardPlayerCharacter::ConfigureImportedVisuals()
{
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (MeshComp == nullptr)
    {
        return;
    }

    USkeletalMesh* CharacterMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Imported/Character/stand.stand"));
    IdleAnimation = LoadObject<UAnimationAsset>(nullptr, TEXT("/Game/Imported/Character/stand_Anim.stand_Anim"));
    WalkAnimation = LoadObject<UAnimationAsset>(nullptr, TEXT("/Game/Imported/Character/walk_Anim.walk_Anim"));
    RunAnimation = LoadObject<UAnimationAsset>(nullptr, TEXT("/Game/Imported/Character/run_Anim.run_Anim"));

    if (CharacterMesh != nullptr)
    {
        MeshComp->SetSkeletalMesh(CharacterMesh);
        MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
        MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
        MeshComp->SetRelativeScale3D(FVector(1.0f));
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        if (IdleAnimation != nullptr)
        {
            MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            MeshComp->SetAnimation(IdleAnimation);
            MeshComp->Play(true);
        }
    }
}

void AMagicShardPlayerCharacter::UpdateVisualAnimation()
{
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (MeshComp == nullptr || LastVisualActionState == ActionState)
    {
        return;
    }

    UAnimationAsset* NextAnimation = IdleAnimation;
    if (ActionState == EMagicShardActionState::Walk)
    {
        NextAnimation = WalkAnimation != nullptr ? WalkAnimation : IdleAnimation;
    }
    else if (ActionState == EMagicShardActionState::Run)
    {
        NextAnimation = RunAnimation != nullptr ? RunAnimation : WalkAnimation;
    }

    if (NextAnimation != nullptr)
    {

        MeshComp->SetAnimation(NextAnimation);
        MeshComp->Play(true);
        LastVisualActionState = ActionState;
    }
}

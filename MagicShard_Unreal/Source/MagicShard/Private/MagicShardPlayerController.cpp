#include "MagicShardPlayerController.h"

#include "MagicShardPlayerCharacter.h"
#include "MagicShardCombatComponent.h"
#include "MagicShardSaveSubsystem.h"

AMagicShardPlayerController::AMagicShardPlayerController()
    : CachedMoveInput(FVector2D::ZeroVector)
{
    bShowMouseCursor = false;
}

void AMagicShardPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAxis(TEXT("MoveForward"), this, &AMagicShardPlayerController::MoveForward);
    InputComponent->BindAxis(TEXT("MoveRight"), this, &AMagicShardPlayerController::MoveRight);
    InputComponent->BindAxis(TEXT("Turn"), this, &AMagicShardPlayerController::Turn);
    InputComponent->BindAxis(TEXT("LookUp"), this, &AMagicShardPlayerController::LookUp);
    InputComponent->BindAxis(TEXT("Zoom"), this, &AMagicShardPlayerController::Zoom);

    InputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AMagicShardPlayerController::StartJump);
    InputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AMagicShardPlayerController::StopJump);
    InputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AMagicShardPlayerController::StartSprint);
    InputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AMagicShardPlayerController::StopSprint);
    InputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &AMagicShardPlayerController::StartAttack);
    InputComponent->BindAction(TEXT("Block"), IE_Pressed, this, &AMagicShardPlayerController::StartBlock);
    InputComponent->BindAction(TEXT("Block"), IE_Released, this, &AMagicShardPlayerController::StopBlock);
    InputComponent->BindAction(TEXT("SaveSlotOne"), IE_Pressed, this, &AMagicShardPlayerController::SaveSlotOne);
    InputComponent->BindAction(TEXT("LoadSlotOne"), IE_Pressed, this, &AMagicShardPlayerController::LoadSlotOne);
    InputComponent->BindAction(TEXT("UpdateSlotOne"), IE_Pressed, this, &AMagicShardPlayerController::UpdateSlotOne);
}

void AMagicShardPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter())
    {
        PlayerCharacter->MoveByInput(CachedMoveInput);
    }
}

void AMagicShardPlayerController::MoveForward(float Value)
{
    CachedMoveInput.Y = Value;
}

void AMagicShardPlayerController::MoveRight(float Value)
{
    CachedMoveInput.X = Value;
}

void AMagicShardPlayerController::Turn(float Value)
{
    if (AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter())
    {
        PlayerCharacter->LookByInput(FVector2D(Value, 0.0f));
    }
}

void AMagicShardPlayerController::LookUp(float Value)
{
    if (AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter())
    {
        PlayerCharacter->LookByInput(FVector2D(0.0f, Value));
    }
}

void AMagicShardPlayerController::Zoom(float Value)
{
    if (AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter())
    {
        PlayerCharacter->ZoomByInput(Value);
    }
}

void AMagicShardPlayerController::StartSprint()
{
    if (AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter())
    {
        PlayerCharacter->BeginSprint();
    }
}

void AMagicShardPlayerController::StopSprint()
{
    if (AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter())
    {
        PlayerCharacter->EndSprint();
    }
}

void AMagicShardPlayerController::StartJump()
{
    if (AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter())
    {
        PlayerCharacter->RequestJump();
    }
}

void AMagicShardPlayerController::StopJump()
{
    if (AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter())
    {
        PlayerCharacter->StopJumpRequest();
    }
}

void AMagicShardPlayerController::StartAttack()
{
    if (AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter())
    {
        if (UMagicShardCombatComponent* CombatComponent = PlayerCharacter->FindComponentByClass<UMagicShardCombatComponent>())
        {
            CombatComponent->TryMeleeAttack();
        }
    }
}

void AMagicShardPlayerController::StartBlock()
{
    if (AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter())
    {
        PlayerCharacter->StartBlock();
    }
}

void AMagicShardPlayerController::StopBlock()
{
    if (AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter())
    {
        PlayerCharacter->StopBlock();
    }
}

void AMagicShardPlayerController::SaveSlotOne()
{
    UGameInstance* GameInstance = GetGameInstance();
    AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter();
    if (GameInstance == nullptr || PlayerCharacter == nullptr)
    {
        return;
    }

    if (UMagicShardSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMagicShardSaveSubsystem>())
    {
        SaveSubsystem->SavePlayerRecord(1, PlayerCharacter->BuildSaveRecord(1));
    }
}

void AMagicShardPlayerController::LoadSlotOne()
{
    UGameInstance* GameInstance = GetGameInstance();
    AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter();
    if (GameInstance == nullptr || PlayerCharacter == nullptr)
    {
        return;
    }

    if (UMagicShardSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMagicShardSaveSubsystem>())
    {
        FMagicShardPlayerRecord Record;
        if (SaveSubsystem->LoadPlayerRecord(1, Record))
        {
            PlayerCharacter->ApplySaveRecord(Record);
        }
    }
}

void AMagicShardPlayerController::UpdateSlotOne()
{
    UGameInstance* GameInstance = GetGameInstance();
    AMagicShardPlayerCharacter* PlayerCharacter = GetMagicShardCharacter();
    if (GameInstance == nullptr || PlayerCharacter == nullptr)
    {
        return;
    }

    if (UMagicShardSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UMagicShardSaveSubsystem>())
    {
        SaveSubsystem->UpdatePlayerRecord(1, PlayerCharacter->BuildSaveRecord(1));
    }
}

AMagicShardPlayerCharacter* AMagicShardPlayerController::GetMagicShardCharacter() const
{
    return Cast<AMagicShardPlayerCharacter>(GetPawn());
}

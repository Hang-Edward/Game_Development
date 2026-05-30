#include "MagicShardPlayerController.h"

#include "MagicShardPlayerCharacter.h"

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

AMagicShardPlayerCharacter* AMagicShardPlayerController::GetMagicShardCharacter() const
{
    return Cast<AMagicShardPlayerCharacter>(GetPawn());
}

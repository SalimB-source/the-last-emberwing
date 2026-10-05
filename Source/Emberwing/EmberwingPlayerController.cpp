#include "EmberwingPlayerController.h"

AEmberwingPlayerController::AEmberwingPlayerController()
{
    bEnableTouchEvents = true;
    bEnableClickEvents = false;
    bShowMouseCursor = false;
}

void AEmberwingPlayerController::BeginPlay()
{
    Super::BeginPlay();

    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
}

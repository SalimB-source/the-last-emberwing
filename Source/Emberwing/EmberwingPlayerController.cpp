#include "EmberwingPlayerController.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"

AEmberwingPlayerController::AEmberwingPlayerController()
{
    bEnableTouchEvents = true;
    bEnableClickEvents = false;
    bShowMouseCursor = false;

    // Correctif camera : laisse UE gerer la camera active et retrouver la CameraComponent du Pawn
    bAutoManageActiveCameraTarget = true;
    bFindCameraComponentWhenViewTarget = true;
    bEnableMouseOverEvents = false;
}

void AEmberwingPlayerController::BeginPlay()
{
    Super::BeginPlay();

    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
    bShowMouseCursor = false;

    // Force le ViewTarget sur le Pawn possede pour eviter ecran noir si WorldSettings n'a pas de CameraActor
    if (APawn* ControlledPawn = GetPawn())
    {
        SetViewTargetWithBlend(ControlledPawn, 0.0f);
    }
    else
    {
        // Si le pawn n'est pas encore possede au BeginPlay (cas PIE retarde), on le fera dans OnPossess
        FTimerHandle TempHandle;
        GetWorldTimerManager().SetTimer(TempHandle, [this]()
        {
            if (APawn* P = GetPawn())
            {
                SetViewTargetWithBlend(P, 0.25f);
            }
        }, 0.2f, false);
    }
}

void AEmberwingPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (InPawn)
    {
        SetViewTargetWithBlend(InPawn, 0.15f);
        // S'assure que la camera du pawn est bien active
        if (UCameraComponent* Cam = InPawn->FindComponentByClass<UCameraComponent>())
        {
            Cam->Activate();
        }
    }
}

void AEmberwingPlayerController::OnUnPossess()
{
    Super::OnUnPossess();
}

void AEmberwingPlayerController::SetViewTarget(AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams)
{
    Super::SetViewTarget(NewViewTarget, TransitionParams);
}

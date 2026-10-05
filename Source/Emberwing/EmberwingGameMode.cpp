#include "EmberwingGameMode.h"

#include "EmberwingCharacter.h"
#include "EmberwingPlayerController.h"
#include "EmberwingPrototypeWorld.h"
#include "Engine/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

AEmberwingGameMode::AEmberwingGameMode()
{
    DefaultPawnClass = AEmberwingCharacter::StaticClass();
    PlayerControllerClass = AEmberwingPlayerController::StaticClass();
}

void AEmberwingGameMode::StartPlay()
{
    UWorld* World = GetWorld();
    if (World)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        World->SpawnActor<AEmberwingPrototypeWorld>(AEmberwingPrototypeWorld::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

        TArray<AActor*> ExistingStarts;
        UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), ExistingStarts);
        if (ExistingStarts.Num() == 0)
        {
            World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(0.0f, 0.0f, 80.0f), FRotator::ZeroRotator, SpawnParams);
        }
    }

    Super::StartPlay();
}

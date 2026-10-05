#include "EmberwingGameMode.h"

#include "Camera/CameraComponent.h"
#include "EmberwingCharacter.h"
#include "EmberwingLightingRig.h"
#include "EmberwingPlayerController.h"
#include "EmberwingPrototypeWorld.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

AEmberwingGameMode::AEmberwingGameMode()
{
    // Tick volontaire : sert uniquement de garde-fou camera/pawn (voir Tick()).
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    DefaultPawnClass = AEmberwingCharacter::StaticClass();
    PlayerControllerClass = AEmberwingPlayerController::StaticClass();
    bStartPlayersAsSpectators = false;
    bDelayedStart = false;
}

void AEmberwingGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    // Lumieres AVANT la geometrie : le rig detruit les lumieres Statique heritees du niveau
    // (invisibles pour Lumen sans "Build Lighting") et pose son post-process. Les meshes
    // crees ensuite sont eclaires par des sources Movable, donc lisibles des la 1re frame.
    if (bInstallLighting)
    {
        AEmberwingLightingRig::EnsureLightingRig(this);
    }

    EnsurePrototypeWorldAndPlayerStart();

    UE_LOG(LogTemp, Display, TEXT("[Emberwing] InitGame sur '%s' - arene + eclairage en place"), *MapName);
}

void AEmberwingGameMode::StartPlay()
{
    EnsurePrototypeWorldAndPlayerStart();

    Super::StartPlay();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC)
        {
            continue;
        }

        if (!PC->GetPawn())
        {
            // Aucun pawn : pas de ViewTarget valide -> ecran noir. On force un restart, puis
            // en dernier recours un spawn direct au debut de la route.
            RestartPlayer(PC);

            if (!PC->GetPawn())
            {
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                const FVector Fallback = AEmberwingPrototypeWorld::GetRouteStartLocation();

                if (APawn* NewPawn = World->SpawnActor<APawn>(DefaultPawnClass, Fallback, FRotator::ZeroRotator, SpawnParams))
                {
                    PC->Possess(NewPawn);
                }
            }
        }

        if (APawn* Pawn = PC->GetPawn())
        {
            PC->SetViewTargetWithBlend(Pawn, 0.0f);
        }
    }
}

void AEmberwingGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Garde-fou : pendant les premieres secondes, on recolle la camera si rien n'est active
    // (cas d'un PlayerStart mal place qui laisserait le view target sur le WorldSettings).
    if (CameraGuardTime > 5.0f)
    {
        return;
    }

    CameraGuardTime += DeltaSeconds;

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        APawn* Pawn = PC ? PC->GetPawn() : nullptr;
        if (!PC || !Pawn)
        {
            continue;
        }

        const bool bHasCamera = (Pawn->FindComponentByClass<UCameraComponent>() != nullptr);
        if (!PC->PlayerCameraManager && bHasCamera)
        {
            PC->SetViewTargetWithBlend(Pawn, 0.0f);
        }
    }
}

AActor* AEmberwingGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (UWorld* World = GetWorld())
    {
        TArray<AActor*> Starts;
        UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), Starts);

        AActor* Best = nullptr;
        float BestDistance = TNumericLimits<float>::Max();
        const FVector Reference = AEmberwingPrototypeWorld::GetRouteStartLocation();

        for (AActor* Start : Starts)
        {
            if (!Start)
            {
                continue;
            }

            // On veut le depart de la route, pas le point le plus haut : un spawn au-dessus
            // de l'autel final enverrait le joueur dans le vide.
            const float Distance = FVector::Dist(Start->GetActorLocation(), Reference);
            if (Distance < BestDistance)
            {
                BestDistance = Distance;
                Best = Start;
            }
        }

        if (Best)
        {
            return Best;
        }
    }

    return Super::ChoosePlayerStart_Implementation(Player);
}

bool AEmberwingGameMode::HasBlockingGroundBelow(const FVector& Location) const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.bTraceComplex = false;

    const FVector Start = Location + FVector(0.0f, 0.0f, 200.0f);
    const FVector End = Location - FVector(0.0f, 0.0f, 900.0f);

    return World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
}

void AEmberwingGameMode::EnsurePrototypeWorldAndPlayerStart()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 1) L'arene, une seule fois. Elle se construit dans son PostInitializeComponents.
    if (bBuildPrototypeWorld)
    {
        TArray<AActor*> ExistingWorlds;
        UGameplayStatics::GetAllActorsOfClass(World, AEmberwingPrototypeWorld::StaticClass(), ExistingWorlds);
        if (ExistingWorlds.Num() == 0)
        {
            World->SpawnActor<AEmberwingPrototypeWorld>(AEmberwingPrototypeWorld::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        }
    }

    // 2) PlayerStart : pose au debut de la route, ou corrige s'il est sous le sol.
    const FVector RouteStart = AEmberwingPrototypeWorld::GetRouteStartLocation();

    TArray<AActor*> ExistingStarts;
    UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), ExistingStarts);

    if (ExistingStarts.Num() == 0)
    {
        APlayerStart* NewStart = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), RouteStart, FRotator(0.0f, 0.0f, 0.0f), SpawnParams);
        if (NewStart)
        {
            NewStart->SetActorLocation(RouteStart);
        }
    }
    else
    {
        for (AActor* Start : ExistingStarts)
        {
            if (!Start)
            {
                continue;
            }

            const FVector Location = Start->GetActorLocation();
            const bool bBelowGround = !HasBlockingGroundBelow(Location);

            if (Location.Z < -400.0f || FVector::Dist2D(Location, FVector(1700.0f, 0.0f, 0.0f)) > 9000.0f || bBelowGround)
            {
                Start->SetActorLocation(RouteStart);
            }
        }
    }

    if (!bBuildPrototypeWorld)
    {
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[Emberwing] PlayerStart pose a %s"), *RouteStart.ToString());
}

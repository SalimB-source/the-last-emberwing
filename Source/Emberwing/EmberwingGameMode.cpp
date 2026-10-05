#include "EmberwingGameMode.h"

#include "EmberwingCharacter.h"
#include "EmberwingPlayerController.h"
#include "EmberwingPrototypeWorld.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/SkyAtmosphere.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

AEmberwingGameMode::AEmberwingGameMode()
{
    DefaultPawnClass = AEmberwingCharacter::StaticClass();
    PlayerControllerClass = AEmberwingPlayerController::StaticClass();
    // Securite : si le Blueprint n'est pas encore compile, on garde la classe C++ par defaut
    bStartPlayersAsSpectators = false;
    bDelayedStart = false;
}

void AEmberwingGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    // InitGame est appele avant StartPlay : on peut deja garantir que le monde prototype existe
    // afin que le PlayerStart soit au-dessus d'une plateforme valide.
    EnsurePrototypeWorldAndPlayerStart();
}

void AEmberwingGameMode::StartPlay()
{
    // Garantit que l'eclairage de secours existe avant que la camera ne s'active (evite frame noir)
    EnsureWorldLighting();
    EnsurePrototypeWorldAndPlayerStart();

    Super::StartPlay();

    // Securite post-Super : si aucun Pawn n'a ete spawne (PlayerStart manquant au moment du RestartPlayer)
    if (UWorld* World = GetWorld())
    {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
            if (PC && PC->GetPawn() == nullptr)
            {
                RestartPlayer(PC);
                // Fallback : teleport direct si RestartPlayer echoue
                if (PC->GetPawn() == nullptr)
                {
                    FActorSpawnParameters SpawnParams;
                    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                    FVector SafeLoc = GetSafePlayerStartLocation();
                    APawn* NewPawn = World->SpawnActor<APawn>(DefaultPawnClass, SafeLoc, FRotator::ZeroRotator, SpawnParams);
                    if (NewPawn)
                    {
                        PC->Possess(NewPawn);
                    }
                }
            }
            // Force ViewTarget apres spawn pour eviter camera noire bloquee sur WorldSettings
            if (PC && PC->GetPawn())
            {
                PC->SetViewTargetWithBlend(PC->GetPawn(), 0.0f);
            }
        }
    }
}

AActor* AEmberwingGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (UWorld* World = GetWorld())
    {
        TArray<AActor*> Starts;
        UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), Starts);
        if (Starts.Num() > 0)
        {
            // Prefere le PlayerStart le plus haut / le plus proche du centre de l'arene
            AActor* Best = Starts[0];
            float BestScore = -FLT_MAX;
            FVector ArenaCenter = FVector(700.0f, 0.0f, 0.0f);
            for (AActor* S : Starts)
            {
                if (!S) continue;
                float DistScore = -FVector::Dist2D(S->GetActorLocation(), ArenaCenter);
                float HeightScore = S->GetActorLocation().Z * 0.5f;
                float Score = DistScore + HeightScore;
                if (Score > BestScore)
                {
                    BestScore = Score;
                    Best = S;
                }
            }
            return Best;
        }
    }
    return Super::ChoosePlayerStart_Implementation(Player);
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

    // 1) PrototypeWorld : ne le creer qu'une fois
    TArray<AActor*> ExistingWorlds;
    UGameplayStatics::GetAllActorsOfClass(World, AEmberwingPrototypeWorld::StaticClass(), ExistingWorlds);
    if (ExistingWorlds.Num() == 0)
    {
        World->SpawnActor<AEmberwingPrototypeWorld>(AEmberwingPrototypeWorld::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }

    // 2) PlayerStart : place SUR la grande plateforme initiale, pas a 0,0,80 qui etait au bord / sous le sol
    TArray<AActor*> ExistingStarts;
    UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), ExistingStarts);
    if (ExistingStarts.Num() == 0)
    {
        FVector SafeLoc = GetSafePlayerStartLocation();
        FRotator SafeRot = FRotator(0.0f, 0.0f, 0.0f);
        APlayerStart* NewStart = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), SafeLoc, SafeRot, SpawnParams);
        if (NewStart)
        {
            // Evite que la capsule spawn a moitie dans le sol : on ajoute un decalage et on verifie le sol
            NewStart->SetActorLocation(SafeLoc);
        }
    }
    else
    {
        // Corrige les PlayerStart existants mal places (ex : map vide avec start a 0,0,0 sous le sol)
        for (AActor* S : ExistingStarts)
        {
            if (!S) continue;
            FVector Loc = S->GetActorLocation();
            if (Loc.Z < 0.0f || FVector::Dist2D(Loc, FVector(700,0,0)) > 4000.0f)
            {
                S->SetActorLocation(GetSafePlayerStartLocation());
            }
        }
    }
}

FVector AEmberwingGameMode::GetSafePlayerStartLocation() const
{
    // Centre de la premiere plateforme generee dans PrototypeWorld : (700,0,-100) avec echelle 14,8,1
    // Top de la plateforme = -100 + 50 (= demi-cube * scale Z 1) = -50. On spawn a -50 + 120 = 70 au dessus.
    // On ajoute X 700 pour etre bien au centre, pas au bord 0.
    return FVector(700.0f, 0.0f, 120.0f);
}

void AEmberwingGameMode::EnsureWorldLighting()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // DirectionalLight
    TArray<AActor*> DirLights;
    UGameplayStatics::GetAllActorsOfClass(World, ADirectionalLight::StaticClass(), DirLights);
    if (DirLights.Num() == 0)
    {
        ADirectionalLight* DL = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector::ZeroVector, FRotator(-45.0f, 35.0f, 0.0f), SpawnParams);
        if (DL && DL->GetLightComponent())
        {
            DL->GetLightComponent()->SetMobility(EComponentMobility::Movable);
            DL->GetLightComponent()->SetIntensity(10.0f);
            DL->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.98f, 0.92f));
            DL->GetLightComponent()->SetCastShadows(true);
            DL->SetActorRotation(FRotator(-45.0f, 35.0f, 0.0f));
        }
    }
    else
    {
        for (AActor* A : DirLights)
        {
            if (ADirectionalLight* DL = Cast<ADirectionalLight>(A))
            {
                if (DL->GetLightComponent() && DL->GetLightComponent()->Intensity < 1.0f)
                {
                    DL->GetLightComponent()->SetIntensity(7.5f);
                }
            }
        }
    }

    // SkyLight (capture temps reel pour eviter noir sans cubemap bake)
    TArray<AActor*> SkyLights;
    UGameplayStatics::GetAllActorsOfClass(World, ASkyLight::StaticClass(), SkyLights);
    if (SkyLights.Num() == 0)
    {
        ASkyLight* SL = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (SL && SL->GetLightComponent())
        {
            SL->GetLightComponent()->SetMobility(EComponentMobility::Movable);
            SL->GetLightComponent()->SetIntensity(1.2f);
            SL->GetLightComponent()->bRealTimeCapture = true;
        }
    }

    // SkyAtmosphere + Fog pour que la SkyLight ait une source
    TArray<AActor*> SkyAtmos;
    UGameplayStatics::GetAllActorsOfClass(World, ASkyAtmosphere::StaticClass(), SkyAtmos);
    if (SkyAtmos.Num() == 0)
    {
        World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }

    TArray<AActor*> Fogs;
    UGameplayStatics::GetAllActorsOfClass(World, AExponentialHeightFog::StaticClass(), Fogs);
    if (Fogs.Num() == 0)
    {
        AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (Fog && Fog->GetComponent())
        {
            Fog->GetComponent()->FogDensity = 0.015f;
            Fog->GetComponent()->FogHeightFalloff = 0.2f;
        }
    }

    // PostProcessVolume global avec exposition fixe et Lumen lisible
    TArray<AActor*> PPVs;
    UGameplayStatics::GetAllActorsOfClass(World, APostProcessVolume::StaticClass(), PPVs);
    bool bHasUnbound = false;
    for (AActor* A : PPVs)
    {
        if (APostProcessVolume* V = Cast<APostProcessVolume>(A))
        {
            if (V->bUnbound) { bHasUnbound = true; break; }
        }
    }
    if (!bHasUnbound)
    {
        APostProcessVolume* PPV = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (PPV)
        {
            PPV->bUnbound = true;
            PPV->Settings.bOverride_AutoExposureMinBrightness = true;
            PPV->Settings.bOverride_AutoExposureMaxBrightness = true;
            PPV->Settings.AutoExposureMinBrightness = 1.0f;
            PPV->Settings.AutoExposureMaxBrightness = 1.0f;
            PPV->Settings.bOverride_AutoExposureBias = true;
            PPV->Settings.AutoExposureBias = 0.0f;
            PPV->Settings.bOverride_LocalExposureHighlightContrastScale = true;
            PPV->Settings.bOverride_LocalExposureShadowContrastScale = true;
            PPV->Settings.bOverride_AmbientCubemapIntensity = true;
            PPV->Settings.AmbientCubemapIntensity = 0.6f;
            PPV->Settings.bOverride_BloomIntensity = true;
            PPV->Settings.BloomIntensity = 0.35f;
        }
    }
}

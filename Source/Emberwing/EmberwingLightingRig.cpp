#include "EmberwingLightingRig.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "EmberwingProceduralAssets.h"
#include "EmberwingPrototypeWorld.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AEmberwingLightingRig::AEmberwingLightingRig()
{
    // Tick discret : sert a re-verifier l'ecran et a purger les lumieres cassee du niveau
    // qui seraient enregistrees apres notre propre BeginPlay.
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    // --- Lumiere principale ("lune"). La mobilite doit etre forcee ICI : c'est le seul
    //     endroit ou elle est garantie (une composante deja enregistree refuse un vrai
    //     changement de mobilite, et une lumiere Statique dans une scene Lumen non buildee
    //     n'eclaire rien du tout -> ecran noir).
    MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
    MoonLight->SetupAttachment(SceneRoot);
    MoonLight->SetMobility(EComponentMobility::Movable);
    MoonLight->SetRelativeRotation(FRotator(-32.0f, 208.0f, 0.0f));

    // --- Contre-jour froid : remplace l'apport d'un SkyLight qui n'a pas de cubemap.
    FillLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("FillLight"));
    FillLight->SetupAttachment(SceneRoot);
    FillLight->SetMobility(EComponentMobility::Movable);
    FillLight->SetRelativeRotation(FRotator(-12.0f, 28.0f, 0.0f));
    FillLight->CastShadows = 0;

    // --- Renvoi du sol, tres doux, pour que les silhouettes ne soient pas decoupees a plat.
    BounceLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("BounceLight"));
    BounceLight->SetupAttachment(SceneRoot);
    BounceLight->SetMobility(EComponentMobility::Movable);
    BounceLight->SetRelativeRotation(FRotator(38.0f, 196.0f, 0.0f));
    BounceLight->CastShadows = 0;

    // --- Halo tiede au-dessus de la premiere plateforme : meme sans aucune autre lumiere,
    //     le point de depart du run reste visible.
    ArenaHalo = CreateDefaultSubobject<UPointLightComponent>(TEXT("ArenaHalo"));
    ArenaHalo->SetupAttachment(SceneRoot);
    ArenaHalo->SetMobility(EComponentMobility::Movable);
    ArenaHalo->SetRelativeLocation(FVector(900.0f, 0.0f, 560.0f));
    ArenaHalo->CastShadows = 0;
}

void AEmberwingLightingRig::BeginPlay()
{
    Super::BeginPlay();

    if (bNeutralizeLevelLights)
    {
        NeutralizeBrokenLevelLights();
    }

    // Reglages appliques ici et non dans le constructeur : les composantes sont enregistrees,
    // les setters propagent donc l'etat au rendu immediatement.
    if (MoonLight)
    {
        MoonLight->SetRelativeRotation(FRotator(MoonPitch, MoonYaw, 0.0f));
        MoonLight->SetIntensity(MoonIntensity);
        MoonLight->SetLightColor(FLinearColor(0.72f, 0.84f, 1.0f), false);
    }

    if (FillLight)
    {
        FillLight->SetIntensity(FillIntensity);
        FillLight->SetLightColor(FLinearColor(0.30f, 0.50f, 0.85f), false);
    }

    if (BounceLight)
    {
        BounceLight->SetIntensity(BounceIntensity);
        BounceLight->SetLightColor(FLinearColor(0.28f, 0.40f, 0.36f), false);
    }

    if (ArenaHalo)
    {
        ArenaHalo->SetIntensity(5200.0f);
        ArenaHalo->SetSoftSourceRadius(180.0f);
        ArenaHalo->SetLightColor(FLinearColor(1.0f, 0.55f, 0.22f), false);
        ArenaHalo->SetUseInverseSquaredFalloff(false);
        ArenaHalo->SetLightFalloffExponent(3.0f);
    }

    ConfigureFogAndSky();
    ConfigurePostProcess();
    PrintDiagnostics();
}

void AEmberwingLightingRig::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    DiagnosticCooldown -= DeltaSeconds;
    if (DiagnosticCooldown <= 0.0f)
    {
        DiagnosticCooldown = 6.0f;

        if (bNeutralizeLevelLights)
        {
            NeutralizeBrokenLevelLights();
        }

        if (bPrintDiagnostics)
        {
            PrintDiagnostics();
        }
    }
}

void AEmberwingLightingRig::NeutralizeBrokenLevelLights()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Le rig possede SES PROPRE brouillard / post-process (SpawnParams.Owner = this) : il ne
    // faut surtout pas les detruire au 2e passage de cette fonction (sinon l'expo figee saute
    // 6 secondes apres le lancement).
    auto IsOwnedByRig = [this](const AActor* Actor) -> bool
    {
        return Actor
            && (Actor == FogActor.Get() || Actor == PostProcess.Get() || Actor == SkyAtmosphereActor.Get() || Actor->GetOwner() == this);
    };

    auto DestroyAllOf = [World, IsOwnedByRig](UClass* ActorClass)
    {
        TArray<AActor*> Found;
        UGameplayStatics::GetAllActorsOfClass(World, ActorClass, Found);
        int32 Removed = 0;

        for (AActor* Actor : Found)
        {
            if (!Actor || Actor->IsActorBeingDestroyed() || IsOwnedByRig(Actor))
            {
                continue;
            }

            if (Actor->Destroy())
            {
                ++Removed;
            }
        }

        return Removed;
    };

    // 1) DirectionalLight posees dans le niveau : en mobilite Statique + Lumen sans "Build
    //    Lighting", elles ne contribuent pas du tout. Le rig fournit sa propre lune movable.
    const int32 RemovedDirectional = DestroyAllOf(ADirectionalLight::StaticClass());

    // 2) Les 2 SkyLight de Prototype.umap n'ont aucun cubemap source (l'icone S_LightError
    //    est serialisee dans le .umap) : elles ne donnent aucune ambiance.
    const int32 RemovedSkylight = DestroyAllOf(ASkyLight::StaticClass());

    // 3) Brouillard et PostProcess du niveau : un FogDensity "joli" herite d'un autre projet
    //    peut etouffer toute la scene a 300 units. Le rig les regenere a des valeurs lisibles.
    const int32 RemovedFog = DestroyAllOf(AExponentialHeightFog::StaticClass());
    const int32 RemovedPostProcess = DestroyAllOf(APostProcessVolume::StaticClass());

    if (bPrintDiagnostics && (RemovedDirectional + RemovedSkylight + RemovedFog + RemovedPostProcess) > 0)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[Emberwing] eclairage du niveau neutralise : %d DirectionalLight, %d SkyLight, %d Fog, %d PostProcessVolume"),
            RemovedDirectional, RemovedSkylight, RemovedFog, RemovedPostProcess);
    }
}

void AEmberwingLightingRig::ConfigureFogAndSky()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.Owner = this;

    if (!SkyAtmosphereActor)
    {
        SkyAtmosphereActor = World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }

    if (!FogActor)
    {
        FogActor = World->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), FVector(1600.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParams);
        if (FogActor && FogActor->GetComponent())
        {
            UExponentialHeightFogComponent* FogComp = FogActor->GetComponent();
            FogComp->SetVolumetricFog(false);
            FogComp->SetFogDensity(FogDensity);
            FogComp->SetFogHeightFalloff(0.10f);
            FogComp->SetFogMaxOpacity(FogMaxOpacity);
        }
    }
}

void AEmberwingLightingRig::ConfigurePostProcess()
{
    UWorld* World = GetWorld();
    if (!World || !bForceReadableExposure)
    {
        return;
    }

    if (!PostProcess)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        SpawnParams.Owner = this;
        PostProcess = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }

    if (!PostProcess)
    {
        return;
    }

    PostProcess->bUnbound = true;

    FPostProcessSettings& Settings = PostProcess->Settings;

    // Exposition figee min = max : plus aucune auto-exposition ne peut plonger l'ecran dans le noir.
    Settings.bOverride_AutoExposureMinBrightness = true;
    Settings.AutoExposureMinBrightness = ExposureBrightness;
    Settings.bOverride_AutoExposureMaxBrightness = true;
    Settings.AutoExposureMaxBrightness = ExposureBrightness;
    Settings.bOverride_AutoExposureBias = true;
    Settings.AutoExposureBias = 0.0f;
    Settings.bOverride_AutoExposureLowPercent = true;
    Settings.AutoExposureLowPercent = 20.0f;
    Settings.bOverride_AutoExposureHighPercent = true;
    Settings.AutoExposureHighPercent = 90.0f;

    Settings.bOverride_BloomIntensity = true;
    Settings.BloomIntensity = 0.22f;
    Settings.bOverride_VignetteIntensity = true;
    Settings.VignetteIntensity = 0.26f;
    Settings.bOverride_MotionBlurAmount = true;
    Settings.MotionBlurAmount = 0.0f;
}

void AEmberwingLightingRig::PrintDiagnostics() const
{
    if (!bPrintDiagnostics || !GEngine)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    int32 MovableDirectional = 0;
    TArray<AActor*> Lights;

    UGameplayStatics::GetAllActorsOfClass(World, ASkyLight::StaticClass(), Lights);
    const int32 SkylightCount = Lights.Num();

    Lights.Empty();
    UGameplayStatics::GetAllActorsOfClass(World, AEmberwingPrototypeWorld::StaticClass(), Lights);
    const int32 ArenaCount = Lights.Num();

    if (MoonLight && MoonLight->GetMobility() == EComponentMobility::Movable)
    {
        ++MovableDirectional;
    }

    FString PawnInfo = TEXT("pawn: aucun (le GameMode n'a pas pu en creer)");
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            const FVector Location = Pawn->GetActorLocation();
            PawnInfo = FString::Printf(TEXT("pawn: X=%.0f Y=%.0f Z=%.0f"), Location.X, Location.Y, Location.Z);
        }
    }

    GEngine->AddOnScreenDebugMessage(91001, 8.0f, FColor(120, 255, 170),
        FString::Printf(TEXT("[EMBERWING] module C++ OK - lune=%d movable - skylight restant=%d - arene=%d"),
            MovableDirectional, SkylightCount, ArenaCount));
    GEngine->AddOnScreenDebugMessage(91002, 8.0f, FColor::Yellow, PawnInfo);
    GEngine->AddOnScreenDebugMessage(91003, 8.0f,
        FEmberwingAssets::AreEnginePrimitivesAvailable() ? FColor::Cyan : FColor::Red,
        FString::Printf(TEXT("[EMBERWING] BasicShapes: %s - materiau: %s"),
            FEmberwingAssets::AreEnginePrimitivesAvailable() ? TEXT("OK") : TEXT("MANQUANT"),
            *FEmberwingAssets::DescribeMaterialSource()));
}

AEmberwingLightingRig* AEmberwingLightingRig::EnsureLightingRig(UObject* WorldContextObject)
{
    UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    if (!World)
    {
        return nullptr;
    }

    TArray<AActor*> Existing;
    UGameplayStatics::GetAllActorsOfClass(World, AEmberwingLightingRig::StaticClass(), Existing);
    for (AActor* Actor : Existing)
    {
        if (IsValid(Actor))
        {
            return Cast<AEmberwingLightingRig>(Actor);
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    return World->SpawnActor<AEmberwingLightingRig>(AEmberwingLightingRig::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
}

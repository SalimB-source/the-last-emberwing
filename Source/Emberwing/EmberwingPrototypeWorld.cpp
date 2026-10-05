#include "EmberwingPrototypeWorld.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EmberwingEnemy.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyAtmosphere.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AEmberwingPrototypeWorld::AEmberwingPrototypeWorld()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        CubeMesh = CubeFinder.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereFinder.Succeeded())
    {
        SphereMesh = SphereFinder.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GridMatFinder(TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
    if (GridMatFinder.Succeeded())
    {
        GridMaterial = GridMatFinder.Object;
    }
    else
    {
        GridMaterial = nullptr;
    }
}

void AEmberwingPrototypeWorld::BeginPlay()
{
    Super::BeginPlay();

    // Correctif Lumiere : si la map n'a pas d'eclairage bake ou si les SkyLight sont en erreur, on cree un eclairage de secours
    // Doit etre fait AVANT d'ajouter les plateformes pour que celles-ci recoivent l'eclairage dynamique.
    EnsureWorldLighting();
    FixExistingWorldLighting();

    // The first slice is generated from simple engine primitives so the project
    // is playable before final environment art and Blueprints are imported.
    AddPlatform(FVector(700.0f, 0.0f, -100.0f), FVector(14.0f, 8.0f, 1.0f));
    AddPlatform(FVector(1420.0f, 0.0f, 80.0f), FVector(4.0f, 5.0f, 0.55f));
    AddPlatform(FVector(1900.0f, 260.0f, 230.0f), FVector(4.5f, 3.2f, 0.55f));
    AddPlatform(FVector(2420.0f, -120.0f, 390.0f), FVector(4.0f, 3.8f, 0.55f));
    AddPlatform(FVector(3000.0f, 80.0f, 560.0f), FVector(6.0f, 5.0f, 0.7f));

    AddLantern(FVector(3000.0f, 80.0f, 760.0f));

    SpawnEnemy(FVector(800.0f, 0.0f, 10.0f));
    SpawnEnemy(FVector(1650.0f, 50.0f, 155.0f));
    SpawnEnemy(FVector(2550.0f, -100.0f, 465.0f));
}

void AEmberwingPrototypeWorld::AddPlatform(const FVector& Location, const FVector& Scale)
{
    if (!CubeMesh)
    {
        return;
    }

    UStaticMeshComponent* Platform = NewObject<UStaticMeshComponent>(this);
    Platform->SetupAttachment(SceneRoot);
    Platform->SetStaticMesh(CubeMesh);
    Platform->SetRelativeLocation(Location);
    Platform->SetRelativeScale3D(Scale);
    Platform->SetMobility(EComponentMobility::Movable);
    Platform->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Platform->SetCastShadow(true);
    Platform->bCastDynamicShadow = true;
    Platform->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    if (GridMaterial)
    {
        Platform->SetMaterial(0, GridMaterial);
    }
    Platform->RegisterComponent();
}

void AEmberwingPrototypeWorld::EnsureWorldLighting()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // DirectionalLight de secours (lumiere principale)
    TArray<AActor*> DirLights;
    UGameplayStatics::GetAllActorsOfClass(World, ADirectionalLight::StaticClass(), DirLights);
    if (DirLights.Num() == 0)
    {
        ADirectionalLight* DL = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0, 0, 800), FRotator(-48.0f, 32.0f, 0.0f), SpawnParams);
        if (DL && DL->GetLightComponent())
        {
            DL->GetLightComponent()->SetMobility(EComponentMobility::Movable);
            DL->GetLightComponent()->SetIntensity(10.0f);
            DL->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.98f, 0.92f));
            DL->GetLightComponent()->SetCastShadows(true);
            DL->GetLightComponent()->SetCastTranslucentShadows(true);
        }
    }

    // SkyLight temps reel (evite noir si cubemap manquant)
    TArray<AActor*> SkyLights;
    UGameplayStatics::GetAllActorsOfClass(World, ASkyLight::StaticClass(), SkyLights);
    if (SkyLights.Num() == 0)
    {
        ASkyLight* SL = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (SL && SL->GetLightComponent())
        {
            SL->GetLightComponent()->SetMobility(EComponentMobility::Movable);
            SL->GetLightComponent()->SetIntensity(1.1f);
            SL->GetLightComponent()->bRealTimeCapture = true;
            SL->GetLightComponent()->SourceType = SLS_CapturedScene;
        }
    }

    // SkyAtmosphere pour que la lumiere ait un ciel
    TArray<AActor*> Atmos;
    UGameplayStatics::GetAllActorsOfClass(World, ASkyAtmosphere::StaticClass(), Atmos);
    if (Atmos.Num() == 0)
    {
        World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }

    // Fog
    TArray<AActor*> Fogs;
    UGameplayStatics::GetAllActorsOfClass(World, AExponentialHeightFog::StaticClass(), Fogs);
    if (Fogs.Num() == 0)
    {
        AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (Fog && Fog->GetComponent())
        {
            Fog->GetComponent()->FogDensity = 0.012f;
            Fog->GetComponent()->FogHeightFalloff = 0.18f;
            Fog->GetComponent()->SetVolumetricFog(true);
        }
    }

    // PostProcessVolume unbound avec exposition fixe
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
            PPV->Settings.AmbientCubemapIntensity = 0.55f;
            PPV->Settings.bOverride_BloomIntensity = true;
            PPV->Settings.BloomIntensity = 0.3f;
        }
    }
}

void AEmberwingPrototypeWorld::FixExistingWorldLighting()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Corrige les lumieres existantes dans la map qui auraient une intensite nulle / capture desactivee
    TArray<AActor*> DirLights;
    UGameplayStatics::GetAllActorsOfClass(World, ADirectionalLight::StaticClass(), DirLights);
    for (AActor* A : DirLights)
    {
        if (ADirectionalLight* DL = Cast<ADirectionalLight>(A))
        {
            if (DL->GetLightComponent())
            {
                if (DL->GetLightComponent()->Intensity < 0.5f)
                {
                    DL->GetLightComponent()->SetIntensity(8.0f);
                }
                DL->GetLightComponent()->SetMobility(EComponentMobility::Movable);
                DL->GetLightComponent()->SetCastShadows(true);
            }
        }
    }

    TArray<AActor*> SkyLights;
    UGameplayStatics::GetAllActorsOfClass(World, ASkyLight::StaticClass(), SkyLights);
    for (AActor* A : SkyLights)
    {
        if (ASkyLight* SL = Cast<ASkyLight>(A))
        {
            if (SL->GetLightComponent())
            {
                SL->GetLightComponent()->bRealTimeCapture = true;
                if (SL->GetLightComponent()->Intensity < 0.2f)
                {
                    SL->GetLightComponent()->SetIntensity(1.0f);
                }
            }
        }
    }
}

void AEmberwingPrototypeWorld::AddLantern(const FVector& Location)
{
    if (SphereMesh)
    {
        UStaticMeshComponent* Lantern = NewObject<UStaticMeshComponent>(this);
        Lantern->SetupAttachment(SceneRoot);
        Lantern->SetStaticMesh(SphereMesh);
        Lantern->SetRelativeLocation(Location);
        Lantern->SetRelativeScale3D(FVector(0.65f));
        Lantern->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Lantern->SetCastShadow(false);
        if (GridMaterial)
        {
            Lantern->SetMaterial(0, GridMaterial);
        }
        Lantern->RegisterComponent();
    }

    UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
    Light->SetupAttachment(SceneRoot);
    Light->SetRelativeLocation(Location);
    Light->SetMobility(EComponentMobility::Movable);
    // Intensite renforcee pour etre visible meme avec Lumen et exposition fixe
    Light->SetIntensity(4200.0f);
    Light->SetAttenuationRadius(2400.0f);
    Light->SetSourceRadius(22.0f);
    Light->SetSoftSourceRadius(48.0f);
    Light->SetLightColor(FLinearColor(1.0f, 0.42f, 0.08f));
    Light->SetCastShadows(true);
    Light->SetUseInverseSquaredFalloff(false);
    Light->RegisterComponent();
}

void AEmberwingPrototypeWorld::SpawnEnemy(const FVector& Location)
{
    if (!GetWorld())
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    GetWorld()->SpawnActor<AEmberwingEnemy>(AEmberwingEnemy::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
}

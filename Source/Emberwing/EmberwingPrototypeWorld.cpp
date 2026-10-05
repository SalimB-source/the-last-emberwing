#include "EmberwingPrototypeWorld.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EmberwingEnemy.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
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
}

void AEmberwingPrototypeWorld::BeginPlay()
{
    Super::BeginPlay();

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
    Platform->RegisterComponent();
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
        Lantern->RegisterComponent();
    }

    UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
    Light->SetupAttachment(SceneRoot);
    Light->SetRelativeLocation(Location);
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetIntensity(1100.0f);
    Light->SetAttenuationRadius(700.0f);
    Light->SetLightColor(FLinearColor(1.0f, 0.36f, 0.08f));
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

#include "EmberwingPrototypeWorld.h"

#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EmberwingEnemy.h"
#include "EmberwingLightingRig.h"
#include "EmberwingProceduralAssets.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    /** Le Cube de /Engine/BasicShapes fait 100 units de cote : 1.0 d'echelle = 100 cm. */
    constexpr float BasicCubeSize = 100.0f;

    FLinearColor LanternGlassColor()
    {
        return FLinearColor(1.0f, 0.46f, 0.14f);
    }
}

AEmberwingPrototypeWorld::AEmberwingPrototypeWorld()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

FVector AEmberwingPrototypeWorld::GetRouteStartLocation()
{
    // Premier plateau : centre (700, 0, -100), echelle Z 1.0 => 100 d'epaisseur,
    // donc dessus a -50. Le capsule du personnage fait 88 de demi-hauteur :
    // -50 + 88 + 22 de marge = 60.
    return FVector(700.0f, 0.0f, 60.0f);
}

FVector AEmberwingPrototypeWorld::GetShrineLocation()
{
    return FVector(3000.0f, 80.0f, 595.0f);
}

void AEmberwingPrototypeWorld::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // IMPORTANT : la geometrie est montee ici et non dans BeginPlay, pour que le sol existe
    // deja quand le GameMode place le pawn (sinon le joueur traverse la scene et tombe).
    AddGround();
    AddRoute();
    AddRoots();
    AddShrine();

    if (bSpawnEnemies)
    {
        SpawnEnemy(FVector(1180.0f, 0.0f, 140.0f));
        SpawnEnemy(FVector(1620.0f, 40.0f, 210.0f));
        SpawnEnemy(FVector(2480.0f, -110.0f, 480.0f));
    }
}

void AEmberwingPrototypeWorld::BeginPlay()
{
    Super::BeginPlay();

    // Le rig de lumiere est idempotent : appele ici aussi, pour que l'arene reste eclairee
    // meme lancee depuis un autre GameMode.
    AEmberwingLightingRig::EnsureLightingRig(this);
}

UStaticMeshComponent* AEmberwingPrototypeWorld::AddMesh(USceneComponent* Parent, EEmberwingPrimitive Primitive, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color, float EmissiveStrength, bool bBlockMovement, bool bCastShadow)
{
    UStaticMesh* Mesh = FEmberwingAssets::GetPrimitive(Primitive);
    if (!Mesh || !Parent)
    {
        return nullptr;
    }

    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
    AddInstanceComponent(Part);
    Part->SetStaticMesh(Mesh);
    Part->SetupAttachment(Parent);
    Part->SetRelativeLocation(Location);
    Part->SetRelativeRotation(Rotation);
    Part->SetRelativeScale3D(Scale);
    Part->SetMobility(EComponentMobility::Movable);
    Part->CastShadow = bCastShadow ? 1 : 0;
    Part->bCastDynamicShadow = bCastShadow ? 1 : 0;

    if (bBlockMovement)
    {
        Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        // Nom litteral du profil integrate au moteur : evite une dependance d'include
        // (UCollisionProfile est dans "CollisionProfile.h", pas dans Engine/).
        Part->SetCollisionProfileName(TEXT("BlockAll"));
    }
    else
    {
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    FEmberwingAssets::ApplyTint(Part, Color, EmissiveStrength);
    Part->RegisterComponent();

    return Part;
}

void AEmberwingPrototypeWorld::AddGround()
{
    // Grande dalle sous tout le run : le BSP du niveau n'est pas une surface de collision
    // fiable, et sans sol de secours le joueur tombe dans le vide (ecran noir garanti).
    AddMesh(SceneRoot, EEmberwingPrimitive::Cube, FVector(1700.0f, 0.0f, -700.0f), FRotator::ZeroRotator,
        FVector(110.0f, 84.0f, 4.0f), GroundColor, 0.0f, true, false);

    // Motte de mousse claire sur le sol : donne une echelle et un repere de depart.
    AddMesh(SceneRoot, EEmberwingPrimitive::Cube, FVector(700.0f, 0.0f, -490.0f), FRotator(0.0f, 0.0f, 12.0f),
        FVector(26.0f, 18.0f, 0.4f), MossColor, 0.0f, false, false);
}

void AEmberwingPrototypeWorld::AddRoute()
{
    struct FPlatformDef
    {
        FVector Location;
        FVector Scale;
    };

    // Le run monte de -50 a +615 : course, saut, puis plane.
    const FPlatformDef Platforms[] = {
        { FVector(700.0f, 0.0f, -100.0f), FVector(14.0f, 8.0f, 1.0f) },
        { FVector(1420.0f, 0.0f, 80.0f), FVector(4.6f, 5.0f, 0.6f) },
        { FVector(1900.0f, 260.0f, 230.0f), FVector(4.6f, 3.4f, 0.6f) },
        { FVector(2420.0f, -120.0f, 390.0f), FVector(4.4f, 4.0f, 0.6f) },
        { FVector(3000.0f, 80.0f, 560.0f), FVector(7.0f, 5.6f, 0.7f) },
    };

    for (const FPlatformDef& Def : Platforms)
    {
        // Le bloc principal (pierre).
        AddMesh(SceneRoot, EEmberwingPrimitive::Cube, Def.Location, FRotator(0.0f, 0.0f, FMath::FRandRange(-4.0f, 4.0f)),
            Def.Scale, StoneColor, 0.0f, true);

        // Plateau mousseux legerement plus grand : casse la lecture "boite parfaitement droite".
        const FVector TopLocation = Def.Location + FVector(0.0f, 0.0f, Def.Scale.Z * BasicCubeSize * 0.5f);
        AddMesh(SceneRoot, EEmberwingPrimitive::Cube, TopLocation, FRotator(0.0f, 0.0f, 2.5f),
            FVector(Def.Scale.X * 1.04f, Def.Scale.Y * 1.04f, Def.Scale.Z * 0.12f), MossColor, 0.0f, false);

        // Racines qui tiennent les plateformes : signent le "Drowned Root".
        const FVector SupportLocation = Def.Location - FVector(0.0f, 0.0f, Def.Scale.Z * BasicCubeSize * 2.4f);
        AddMesh(SceneRoot, EEmberwingPrimitive::Cylinder, SupportLocation, FRotator(0.0f, 0.0f, 18.0f),
            FVector(Def.Scale.X * 0.22f, Def.Scale.Y * 0.22f, Def.Scale.Z * 4.8f), RootColor, 0.0f, false);
    }

    // Lanternes d'ember le long de la route (l'autel final a sa propre lumiere).
    AddLantern(FVector(1050.0f, 0.0f, 0.0f), 190.0f);
    AddLantern(FVector(1660.0f, 130.0f, 300.0f), 210.0f);
    AddLantern(FVector(2180.0f, -40.0f, 470.0f), 220.0f);
    AddLantern(FVector(2700.0f, 40.0f, 640.0f), 230.0f);
    AddLantern(FVector(700.0f, 0.0f, 150.0f), 260.0f);
}

void AEmberwingPrototypeWorld::AddRoots()
{
    // Couronne de racines-mortes autour de l'arene : le regard a toujours de la geometrie
    // en perspective, au lieu du vide noir.
    const FVector Center(1700.0f, 0.0f, 0.0f);
    const float Radius = 5200.0f;

    for (int32 Index = 0; Index < 9; ++Index)
    {
        const float Angle = (360.0f / 9.0f) * Index + 11.0f;
        const float Height = 14.0f + (Index % 3) * 5.0f;

        const FVector Location = Center + FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            Height * BasicCubeSize * 0.5f - 600.0f);

        AddMesh(SceneRoot, EEmberwingPrimitive::Cylinder, Location, FRotator(0.0f, 0.0f, Angle + 90.0f),
            FVector(1.5f + (Index % 2) * 0.5f, 1.5f + (Index % 2) * 0.5f, Height), RootColor, 0.0f, false, false);

        // Chevelu : un cone couche au sommet, pour casser la silhouette cylindrique.
        AddMesh(SceneRoot, EEmberwingPrimitive::Cone, Location + FVector(0.0f, 0.0f, Height * BasicCubeSize * 0.5f),
            FRotator(0.0f, Angle, 26.0f), FVector(2.2f, 2.2f, 3.4f), RootColor, 0.0f, false, false);
    }
}

void AEmberwingPrototypeWorld::AddLantern(const FVector& Location, float Height)
{
    // Verre de la lanterne : sphere emissive (le halo vient du materiau, pas d'un asset).
    AddMesh(SceneRoot, EEmberwingPrimitive::Sphere, Location + FVector(0.0f, 0.0f, Height), FRotator::ZeroRotator,
        FVector(0.42f), LanternGlassColor(), 9.0f, false);

    // Armature : deux anneaux.
    AddMesh(SceneRoot, EEmberwingPrimitive::Torus, Location + FVector(0.0f, 0.0f, Height + 22.0f), FRotator(90.0f, 0.0f, 0.0f),
        FVector(0.55f, 0.55f, 0.55f), RootColor, 0.0f, false);
    AddMesh(SceneRoot, EEmberwingPrimitive::Cone, Location + FVector(0.0f, 0.0f, Height + 46.0f), FRotator(0.0f, 0.0f, 0.0f),
        FVector(0.55f, 0.55f, 0.40f), RootColor, 0.0f, false);

    // Attache : fin cube vertical.
    AddMesh(SceneRoot, EEmberwingPrimitive::Cube, Location + FVector(0.0f, 0.0f, Height * 0.5f - 20.0f), FRotator::ZeroRotator,
        FVector(0.06f, 0.06f, Height * 0.009f), RootColor, 0.0f, false);

    UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
    AddInstanceComponent(Light);
    Light->SetupAttachment(SceneRoot);
    Light->SetRelativeLocation(Location + FVector(0.0f, 0.0f, Height));
    Light->SetMobility(EComponentMobility::Movable);
    Light->CastShadows = 0;
    Light->bAffectsWorld = 1;
    // Falloff doux non-inverse-carre : la lanterne reste lisible meme avec une exposition figee.
    Light->SetIntensity(LanternIntensity);
    Light->SetLightColor(FLinearColor(1.0f, 0.50f, 0.18f), false);
    Light->SetSourceRadius(18.0f);
    Light->SetSoftSourceRadius(52.0f);
    Light->SetUseInverseSquaredFalloff(false);
    Light->SetLightFalloffExponent(2.6f);
    Light->RegisterComponent();
}

void AEmberwingPrototypeWorld::AddShrine()
{
    const FVector Location = GetShrineLocation();

    // Pedestal
    AddMesh(SceneRoot, EEmberwingPrimitive::Cylinder, Location - FVector(0.0f, 0.0f, 55.0f), FRotator::ZeroRotator,
        FVector(3.2f, 3.2f, 1.1f), StoneColor, 0.0f, true);
    AddMesh(SceneRoot, EEmberwingPrimitive::Torus, Location + FVector(0.0f, 0.0f, 10.0f), FRotator(90.0f, 0.0f, 0.0f),
        FVector(2.6f, 2.6f, 2.6f), MossColor, 0.0f, false);

    // L'ember a recuperer : sphere emissive + anneau qui la suit visuellement.
    AddMesh(SceneRoot, EEmberwingPrimitive::Sphere, Location + FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator,
        FVector(0.85f), LanternGlassColor(), 22.0f, false);
    AddMesh(SceneRoot, EEmberwingPrimitive::Torus, Location + FVector(0.0f, 0.0f, 120.0f), FRotator(90.0f, 0.0f, 0.0f),
        FVector(1.5f, 1.5f, 1.5f), FLinearColor(0.9f, 0.62f, 0.24f), 3.0f, false);

    UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
    AddInstanceComponent(Light);
    Light->SetupAttachment(SceneRoot);
    Light->SetRelativeLocation(Location + FVector(0.0f, 0.0f, 150.0f));
    Light->SetMobility(EComponentMobility::Movable);
    Light->CastShadows = 0;
    Light->SetIntensity(LanternIntensity * 2.4f);
    Light->SetLightColor(FLinearColor(1.0f, 0.56f, 0.22f), false);
    Light->SetSoftSourceRadius(90.0f);
    Light->SetUseInverseSquaredFalloff(false);
    Light->SetLightFalloffExponent(2.4f);
    Light->RegisterComponent();
}

void AEmberwingPrototypeWorld::SpawnEnemy(const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    World->SpawnActor<AEmberwingEnemy>(AEmberwingEnemy::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
}

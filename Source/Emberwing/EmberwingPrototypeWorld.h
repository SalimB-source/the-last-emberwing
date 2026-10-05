#pragma once

#include "CoreMinimal.h"
#include "EmberwingProceduralAssets.h"
#include "GameFramework/Actor.h"
#include "EmberwingPrototypeWorld.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * Arene du prototype : sol, route de plateformes, racines, lanternes d'ember et autel.
 *
 * Tout est genere a partir des primitives du moteur (aucun .uasset dans le depot), et la
 * construction a lieu dans PostInitializeComponents : la geometrie doit exister AVANT que
 * le GameMode ne positionne le pawn, sinon le joueur traverse le sol.
 */
UCLASS()
class EMBERWING_API AEmberwingPrototypeWorld : public AActor
{
    GENERATED_BODY()

public:
    AEmberwingPrototypeWorld();

    /** Entree du run : au centre de la premiere plateforme, au-dessus de son plateau. */
    static FVector GetRouteStartLocation();

    /** Dernier autel (objectif du prototype). */
    static FVector GetShrineLocation();

protected:
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;

private:
    UStaticMeshComponent* AddMesh(USceneComponent* Parent, EEmberwingPrimitive Primitive, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color, float EmissiveStrength, bool bBlockMovement);

    void AddGround();
    void AddRoute();
    void AddRoots();
    void AddLantern(const FVector& Location, float Height);
    void AddShrine();
    void SpawnEnemy(const FVector& Location);

    UPROPERTY(VisibleAnywhere, Category = "Emberwing")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(EditAnywhere, Category = "Emberwing|Decor")
    FLinearColor StoneColor = FLinearColor(0.115f, 0.115f, 0.105f);

    UPROPERTY(EditAnywhere, Category = "Emberwing|Decor")
    FLinearColor MossColor = FLinearColor(0.055f, 0.125f, 0.080f);

    UPROPERTY(EditAnywhere, Category = "Emberwing|Decor")
    FLinearColor GroundColor = FLinearColor(0.045f, 0.055f, 0.050f);

    UPROPERTY(EditAnywhere, Category = "Emberwing|Decor")
    FLinearColor RootColor = FLinearColor(0.075f, 0.055f, 0.040f);

    /** Intensite des lanternes : les lumieres locales sont en units "candela" avec le
     *  falloff non-inverse-carre choisi pour rester lisible sans PostProcessVolume. */
    UPROPERTY(EditAnywhere, Category = "Emberwing|Lights", meta = (ClampMin = "0.0"))
    float LanternIntensity = 2600.0f;

    UPROPERTY(EditAnywhere, Category = "Emberwing|Lights")
    bool bSpawnEnemies = true;
};

#pragma once

#include "CoreMinimal.h"
#include "EmberwingProceduralAssets.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/** Primitives du moteur utilisees pour modeller le personnage, les ennemis et l'arene. */
UENUM(BlueprintType)
enum class EEmberwingPrimitive : uint8
{
	Cube       UMETA(DisplayName = "Cube"),
	Sphere     UMETA(DisplayName = "Sphere"),
	Cylinder   UMETA(DisplayName = "Cylinder"),
	Cone       UMETA(DisplayName = "Cone"),
	Plane      UMETA(DisplayName = "Plane"),
	Torus      UMETA(DisplayName = "Torus")
};

/**
 * Acces aux assets "de secours" du moteur.
 *
 * Le depot ne contient aucun .uasset (voir Content/README.md) : tout le visuel est donc
 * fabule a partir des /Engine/BasicShapes et d'une Material Instance Dynamic.
 * Chaque resolution est tolerante : si un asset manque, on retombe sur un equivalent
 * et l'etat reel est affiche par le diagnostic de la lampe d'embers.
 */
class EMBERWING_API FEmberwingAssets
{
public:
    /** Maille BasicShapes du moteur, ou le cube si la maille demandee est indisponible. */
    static UStaticMesh* GetPrimitive(EEmberwingPrimitive Kind);

    /**
     * Applique une couleur (et une lumiere emise) sur une composante de maille.
     * Retour true si le parametre "Color" du materiau a bien repondu.
     */
    static bool ApplyTint(UStaticMeshComponent* Component, const FLinearColor& Color, float EmissiveStrength);

    /** Le materiau de base reelu (chemin court) - pour le diagnostic. */
    static FString DescribeMaterialSource();

    /** Etat du parametre "Color" : 0 = inconnu, 1 = OK, -1 = absent du materiau. */
    static int32 GetColorParameterState();

    /** Faux si /Engine/BasicShapes n'est pas installe (=> rien ne serait visible). */
    static bool AreEnginePrimitivesAvailable();
};

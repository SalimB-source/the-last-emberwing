#include "EmberwingProceduralAssets.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	/** Une entree par valeur de EEmberwingPrimitive, dans l'ordre de l'enum. */
	const TCHAR* GPrimitivePaths[] =
	{
		TEXT("/Engine/BasicShapes/Cube.Cube"),
		TEXT("/Engine/BasicShapes/Sphere.Sphere"),
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
		TEXT("/Engine/BasicShapes/Cone.Cone"),
		TEXT("/Engine/BasicShapes/Plane.Plane"),
		TEXT("/Engine/BasicShapes/Torus.Torus")
	};

	/**
	 * Materiaux tries par preferance.
	 * M_BasicColor est le materiau a creer une fois dans l'editeur (recette au paragraphe "Couleurs"
	 * du README) : il expose les parametres Color + Emissive, ce qui donne un vrai rendu
	 * emissif aux yeux et au coeur d'ember. Sans lui, on retombe sur un materiau du moteur :
	 * les formes restent visibles mais restent gris neutre.
	 */
	const TCHAR* GMaterialCandidates[] =
	{
		TEXT("/Game/Emberwing/Materials/M_BasicColor.M_BasicColor"),
		TEXT("/Game/Emberwing/M_BasicColor.M_BasicColor"),
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"),
		TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial")
	};

	TWeakObjectPtr<UStaticMesh> GPrimitiveCache[6];
	TWeakObjectPtr<UMaterialInterface> GBaseMaterial;
	FString GBaseMaterialName = TEXT("aucun");
	bool bBaseMaterialResolved = false;
	int8 GColorParameterState = 0;

	UMaterialInterface* ResolveBaseMaterial()
	{
		if (bBaseMaterialResolved)
		{
			return GBaseMaterial.Get();
		}

		bBaseMaterialResolved = true;

		const int32 CandidateCount = static_cast<int32>(sizeof(GMaterialCandidates) / sizeof(GMaterialCandidates[0]));
		for (int32 Index = 0; Index < CandidateCount; ++Index)
		{
			const TCHAR* Candidate = GMaterialCandidates[Index];
			if (UMaterialInterface* Found = LoadObject<UMaterialInterface>(nullptr, Candidate))
			{
				GBaseMaterial = Found;
				GBaseMaterialName = FPaths::GetCleanFilename(FString(Candidate));
				// Les 2 premiers candidats sont le materiau du projet : eux seuls exposent
				// vraiment Color/Emissive. Un materiau du moteur rend les formes grises.
				GColorParameterState = (Index <= 1) ? 1 : -1;
				return Found;
			}
		}

		GColorParameterState = 0;
		return nullptr;
	}
}

UStaticMesh* FEmberwingAssets::GetPrimitive(EEmberwingPrimitive Kind)
{
	const int32 Index = static_cast<int32>(Kind);
	const int32 Count = static_cast<int32>(sizeof(GPrimitivePaths) / sizeof(GPrimitivePaths[0]));

	if (Index >= 0 && Index < Count && GPrimitiveCache[Index].IsValid())
	{
		return GPrimitiveCache[Index].Get();
	}

	const TCHAR* Path = (Index >= 0 && Index < Count) ? GPrimitivePaths[Index] : GPrimitivePaths[0];
	UStaticMesh* Loaded = LoadObject<UStaticMesh>(nullptr, Path);

	// Repli : une maille manquante ne doit jamais vider l'ecran, on utilise le cube.
	if (!Loaded && FCString::Strcmp(Path, GPrimitivePaths[0]) != 0)
	{
		Loaded = LoadObject<UStaticMesh>(nullptr, GPrimitivePaths[0]);
	}

	if (Index >= 0 && Index < Count)
	{
		GPrimitiveCache[Index] = Loaded;
	}

	return Loaded;
}

bool FEmberwingAssets::ApplyTint(UStaticMeshComponent* Component, const FLinearColor& Color, float EmissiveStrength)
{
	if (!Component)
	{
		return false;
	}

	UMaterialInterface* Base = ResolveBaseMaterial();
	if (!Base)
	{
		// Aucun materiau trouve : on laisse celui par defaut de la maille, l'objet reste visible.
		return false;
	}

	UMaterialInstanceDynamic* Mid = Component->CreateDynamicMaterialInstance(0, Base);
	if (!Mid)
	{
		return false;
	}

	// Les setters d'un MID retournent void : un nom de parametre inconnu est silencieusement
	// ignore. On ecrit donc les trois noms courants sans tester, et c'est le MATERIAU resolu
	// qui dit si la couleur a une prise (voir GColorParameterState dans ResolveBaseMaterial).
	Mid->SetVectorParameterValue(TEXT("Color"), Color);
	Mid->SetVectorParameterValue(TEXT("BaseColor"), Color);
	Mid->SetVectorParameterValue(TEXT("Tint"), Color);

	Mid->SetScalarParameterValue(TEXT("Emissive"), EmissiveStrength);
	Mid->SetScalarParameterValue(TEXT("EmissiveStrength"), EmissiveStrength);
	Mid->SetScalarParameterValue(TEXT("Roughness"), 0.55f);

	return GColorParameterState == 1;
}

FString FEmberwingAssets::DescribeMaterialSource()
{
	ResolveBaseMaterial();

	if (GColorParameterState == -1)
	{
		return FString::Printf(TEXT("%s (sans parametre Color -> gris moteur)"), *GBaseMaterialName);
	}

	return GBaseMaterialName;
}

int32 FEmberwingAssets::GetColorParameterState()
{
	ResolveBaseMaterial();
	return GColorParameterState;
}

bool FEmberwingAssets::AreEnginePrimitivesAvailable()
{
	return GetPrimitive(EEmberwingPrimitive::Cube) != nullptr;
}

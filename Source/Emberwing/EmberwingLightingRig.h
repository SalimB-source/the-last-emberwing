#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmberwingLightingRig.generated.h"

class AExponentialHeightFog;
class APostProcessVolume;
class ASkyAtmosphere;
class UDirectionalLightComponent;
class UPointLightComponent;
class USceneComponent;

/**
 * Eclairage complet de l'arene, entierement procedural.
 *
 * Pourquoi ne pas se contenter des lumieres du niveau ?
 *  - Prototype.umap a ete enregistre avec 1 DirectionalLight et 2 SkyLight herites du modele,
 *    sans "Build Lighting" (l'icone /Engine/EditorResources/LightIcons/S_LightError est
 *    serialisee dans le .umap). Les SkyLight n'ont aucun cubemap source : elles n'emettent
 *    rien, et une lumiere en mobilite Statique n'eclaire pas une scene Lumen non buildee.
 *    Resultat : le monde est rendu noir.
 *  - La mobilite d'une composante deja enregistree ne se change pas proprement au runtime :
 *    on la force donc dans le CONSTRUCTEUR, qui est le seul endroit garanti.
 *
 * Ce rig est idempotent (EnsureLightingRig) et ne depend d'aucun .uasset.
 */
UCLASS()
class EMBERWING_API AEmberwingLightingRig : public AActor
{
    GENERATED_BODY()

public:
    AEmberwingLightingRig();

    /** Cree le rig s'il manque, et neutralise les lumieres cassees posees dans le niveau. */
    UFUNCTION(BlueprintCallable, Category = "Emberwing", meta = (WorldContext = "WorldContextObject"))
    static AEmberwingLightingRig* EnsureLightingRig(UObject* WorldContextObject);

    /** Lumiere principale ("lune") : direction = FRotator(MoonPitch, MoonYaw). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lune", meta = (ClampMin = "0.0"))
    float MoonIntensity = 42.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lune", meta = (ClampMin = "-85.0", ClampMax = "-5.0"))
    float MoonPitch = -32.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lune")
    float MoonYaw = 208.0f;

    /** Contre-jour froid, oppose a la lune : remplace l'ambiance que donnerait un SkyLight. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambiance", meta = (ClampMin = "0.0"))
    float FillIntensity = 6.5f;

    /** Renvoi du sol, tres doux, pour que les silhouettes ne soient pas decoupees a plat. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambiance", meta = (ClampMin = "0.0"))
    float BounceIntensity = 2.4f;

    /** Densite volontairement proche du defaut moteur (0.00045) : au-dela, tout devient noir. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brouillard", meta = (ClampMin = "0.0"))
    float FogDensity = 0.00035f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brouillard", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float FogMaxOpacity = 0.72f;

    /** Exposition forcee : 1.0 = image lisible meme sans PostProcessVolume ni AutoExposure. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exposition", meta = (ClampMin = "0.05"))
    float ExposureBrightness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exposition")
    bool bForceReadableExposure = true;

    /** Detruit les lumieres Statique (et leurs doublons) posees dans le niveau. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing")
    bool bNeutralizeLevelLights = true;

    /** Messages a l'ecran : permet de savoir si le module C++ tourne vraiment. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing")
    bool bPrintDiagnostics = true;

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

private:
    void NeutralizeBrokenLevelLights();
    void ConfigureFogAndSky();
    void ConfigurePostProcess();
    void PrintDiagnostics() const;

    UPROPERTY(VisibleAnywhere, Category = "Emberwing")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Emberwing")
    TObjectPtr<UDirectionalLightComponent> MoonLight;

    UPROPERTY(VisibleAnywhere, Category = "Emberwing")
    TObjectPtr<UDirectionalLightComponent> FillLight;

    UPROPERTY(VisibleAnywhere, Category = "Emberwing")
    TObjectPtr<UDirectionalLightComponent> BounceLight;

    UPROPERTY(VisibleAnywhere, Category = "Emberwing")
    TObjectPtr<UPointLightComponent> ArenaHalo;

    UPROPERTY()
    TObjectPtr<AExponentialHeightFog> FogActor;

    UPROPERTY()
    TObjectPtr<APostProcessVolume> PostProcess;

    UPROPERTY()
    TObjectPtr<ASkyAtmosphere> SkyAtmosphereActor;

    float DiagnosticCooldown = 0.0f;
};

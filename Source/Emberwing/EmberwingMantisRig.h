#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EmberwingProceduralAssets.h"
#include "EmberwingMantisRig.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UPointLightComponent;

/** Style de modele procedural partage par Emberwing et ses ennemis. */
UENUM(BlueprintType)
enum class EEmberwingModelStyle : uint8
{
	EmberwingMantis UMETA(DisplayName = "Mante d'ember"),
	DrownedStrider  UMETA(DisplayName = "Patineur noye")
};

/** Une patte : 2 pivots (hanche, genou) + 2 segments. */
USTRUCT()
struct FEmberwingLeg
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<USceneComponent> Hip = nullptr;
	UPROPERTY() TObjectPtr<USceneComponent> Knee = nullptr;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Thigh = nullptr;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Shin = nullptr;

	/** Decalage dans le cycle de marche (radians). */
	float Phase = 0.0f;
	/** +1 a droite, -1 a gauche. */
	float SideSign = 1.0f;
	/** Ecartement au repos de la hanche, en degres. */
	float RestYaw = 0.0f;
	float RestPitch = 0.0f;
	/** Flexion au repos du genou : c'est elle qui pose la patte au sol. */
	float RestKneePitch = 0.0f;
};

/** Un bras raptorial : epaule + coude + avant-bras + faucille. */
USTRUCT()
struct FEmberwingArm
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<USceneComponent> Shoulder = nullptr;
	UPROPERTY() TObjectPtr<USceneComponent> Elbow = nullptr;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ForeArm = nullptr;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Blade = nullptr;

	float SideSign = 1.0f;
};

/**
 * Modele + animation procedurale du mante.
 *
 * Aucun asset n'existe dans le depot (pas de squelette, pas d'anim) : le corps est donc
 * construit avec les primitives du moteur, chaque pivot etant une USceneComponent que l'on
 * anime a la main. Le jour ou un vrai Skeletal Mesh arrive, il suffit de deleter cette
 * composante et de brancher l'anim sur le Mesh du personnage : rien d'autre ne bouge.
 */
UCLASS(ClassGroup = (Emberwing), meta = (BlueprintSpawnableComponent))
class EMBERWING_API UEmberwingMantisRig : public UActorComponent
{
	GENERATED_BODY()

public:
	UEmberwingMantisRig();

	/** Construit le modele sous Parent (normalement le Mesh du Character). */
	UFUNCTION(BlueprintCallable, Category = "Emberwing")
	bool Build(USceneComponent* Parent);

	UFUNCTION(BlueprintCallable, Category = "Emberwing")
	void SetGlide(bool bNewGlide) { bGlide = bNewGlide; }

	UFUNCTION(BlueprintCallable, Category = "Emberwing")
	void PlayAttack();

	UFUNCTION(BlueprintCallable, Category = "Emberwing")
	void FlashHit() { FlashRemaining = 0.18f; }

	/** Lumiere de l'ember, animee avec le battement du coeur du personnage. */
	UFUNCTION(BlueprintCallable, Category = "Emberwing")
	void SetEmberLight(UPointLightComponent* InLight) { EmberLight = InLight; }

	UFUNCTION(BlueprintPure, Category = "Emberwing")
	bool IsRigBuilt() const { return bBuilt; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing")
	EEmberwingModelStyle Style = EEmberwingModelStyle::EmberwingMantis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing")
	FLinearColor ShellColor = FLinearColor(0.045f, 0.150f, 0.115f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing")
	FLinearColor BellyColor = FLinearColor(0.100f, 0.230f, 0.170f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing")
	FLinearColor BladeColor = FLinearColor(0.720f, 0.750f, 0.700f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing")
	FLinearColor EyeColor = FLinearColor(4.0f, 1.60f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing")
	FLinearColor WingColor = FLinearColor(0.30f, 0.52f, 0.55f);

	/** Amplitude globale de l'animation (utile pour un fondu au ralenti). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float AnimationScale = 1.0f;

	/** Intensite de la lumiere portee (braise du joueur / sac d'air de l'ennemi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing", meta = (ClampMin = "0.0"))
	float EmberLightIntensity = 2600.0f;

	/** Duree de l'animation de tranche (le hitbox, lui, reste dans AEmberwingCharacter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing", meta = (ClampMin = "0.1"))
	float AttackDuration = 0.42f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	USceneComponent* AddJoint(USceneComponent* Parent, const FVector& Location, const FRotator& Rotation);
	UStaticMeshComponent* AddPart(USceneComponent* Parent, EEmberwingPrimitive Primitive, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color, float EmissiveStrength, bool bCastShadow);

	void BuildMantis();
	void BuildStrider();
	void AddLeg(USceneComponent* Parent, const FVector& HipLocation, float YawDegrees, float SideSign, float Phase, float LengthScale, float HipPitch, float KneePitch);
	void BuildArms(USceneComponent* Parent);
	void BuildWings(USceneComponent* Parent);

	void AnimateBody(float DeltaTime, float SpeedRatio, bool bGrounded);
	void AnimateLegs(float DeltaTime, float SpeedRatio, bool bGrounded);
	void AnimateArms(float DeltaTime, float SpeedRatio);
	void AnimateWings(float DeltaTime, float SpeedRatio, bool bGrounded);

	UPROPERTY() TObjectPtr<USceneComponent> RootJoint = nullptr;
	UPROPERTY() TObjectPtr<USceneComponent> Pelvis = nullptr;
	UPROPERTY() TObjectPtr<USceneComponent> Thorax = nullptr;
	UPROPERTY() TObjectPtr<USceneComponent> Neck = nullptr;
	UPROPERTY() TObjectPtr<USceneComponent> AbdomenRoot = nullptr;
	UPROPERTY() TObjectPtr<USceneComponent> WingLeftJoint = nullptr;
	UPROPERTY() TObjectPtr<USceneComponent> WingRightJoint = nullptr;

	UPROPERTY() TObjectPtr<UStaticMeshComponent> HeadMesh = nullptr;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> EmberCore = nullptr;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> WingLeft = nullptr;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> WingRight = nullptr;

	UPROPERTY() TArray<USceneComponent*> Joints;
	UPROPERTY() TArray<UStaticMeshComponent*> Parts;
	UPROPERTY() TArray<FEmberwingLeg> Legs;
	UPROPERTY() TArray<FEmberwingArm> Arms;

	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> EmberLight = nullptr;

	bool bBuilt = false;
	bool bGlide = false;

	float GaitPhase = 0.0f;
	float IdleTime = 0.0f;
	float AttackTime = 0.0f;
	float FlashRemaining = 0.0f;
	float FlashPop = 0.0f;
	float SmoothedSpeedRatio = 0.0f;
	float WingSpread = 0.0f;
};

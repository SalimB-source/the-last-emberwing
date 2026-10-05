#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EmberwingEnemy.generated.h"

class UEmberwingMantisRig;
class UPointLightComponent;

/**
 * Patineur des racines noyees : ennemi de base du prototype.
 * Modele et annime par le meme rig procedural que le joueur (style DrownedStrider).
 */
UCLASS()
class EMBERWING_API AEmberwingEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AEmberwingEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "1.0"))
	float MaxHealth = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float ContactDamage = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "100.0"))
	float AggroRadius = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageRange = 155.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Emberwing")
	TObjectPtr<UEmberwingMantisRig> StriderRig;

	/** Sac d'air bioluminescent : rend l'ennemi lisible dans le noir. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Emberwing")
	TObjectPtr<UPointLightComponent> RotGlow;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetHealthRatio() const { return (MaxHealth > KINDA_SMALL_NUMBER) ? (CurrentHealth / MaxHealth) : 0.0f; }

protected:
	virtual void PostInitializeComponents() override;

private:
	float CurrentHealth = 40.0f;
	float NextAttackTime = 0.0f;
};

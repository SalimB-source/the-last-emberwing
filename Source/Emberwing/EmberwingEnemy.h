#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EmberwingEnemy.generated.h"

UCLASS()
class EMBERWING_API AEmberwingEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    AEmberwingEnemy();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    float MaxHealth = 40.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    float ContactDamage = 8.0f;

private:
    UPROPERTY(VisibleAnywhere, Category = "Prototype")
    class UStaticMeshComponent* PlaceholderBody;

    float CurrentHealth = 40.0f;
    float NextAttackTime = 0.0f;
};

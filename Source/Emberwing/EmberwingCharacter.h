#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EmberwingCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class EMBERWING_API AEmberwingCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEmberwingCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void PerformLightAttack();

    UFUNCTION(BlueprintCallable, Category = "Traversal")
    void StartGlide();

    UFUNCTION(BlueprintCallable, Category = "Traversal")
    void StopGlide();

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsAttacking() const { return bIsAttacking; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* FollowCamera;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float GlideGravityScale = 0.28f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackDamage = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackCooldown = 0.35f;

protected:
    virtual void BeginPlay() override;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void AttackPressed();
    void GlidePressed();
    void GlideReleased();

private:
    UPROPERTY(VisibleAnywhere, Category = "Prototype")
    UStaticMeshComponent* PlaceholderBody;

    UPROPERTY(VisibleAnywhere, Category = "Prototype")
    UStaticMeshComponent* PlaceholderBlade;

    bool bIsAttacking = false;
    bool bIsGliding = false;
    float LastAttackTime = -100.0f;
};

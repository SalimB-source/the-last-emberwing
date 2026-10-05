#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EmberwingCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UPointLightComponent;
class UEmberwingMantisRig;

/**
 * Emberwing, le jeune mante.
 *
 * Le corps n'est pas un asset : il est modele et annime par UEmberwingMantisRig a partir des
 * primitives du moteur (voir Content/README.md). Les lumieres, elles, viennent du
 * AEmberwingLightingRig cree par le GameMode + d'une braise portee par le personnage, qui
 * garantit qu'on le voit meme dans une scene sous-eclairee.
 */
UCLASS()
class EMBERWING_API AEmberwingCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEmberwingCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    /** Braise du mante : eclairage de secours qui suit le joueur. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Emberwing")
    TObjectPtr<UPointLightComponent> EmberLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Emberwing")
    TObjectPtr<UEmberwingMantisRig> MantisRig;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void PerformLightAttack();

    UFUNCTION(BlueprintCallable, Category = "Traversal")
    void StartGlide();

    UFUNCTION(BlueprintCallable, Category = "Traversal")
    void StopGlide();

    /** Rebranche ViewTarget + camera : l'ecran noir vient souvent d'un view target perdu. */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void EnsureCameraActive();

    /** Renvoi au dernier appui sur, si le joueur tombe dans le vide. */
    UFUNCTION(BlueprintCallable, Category = "Traversal")
    void ResetFall();

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsAttacking() const { return bIsAttacking; }

    UFUNCTION(BlueprintPure, Category = "Traversal")
    bool IsGliding() const { return bIsGliding; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float GlideGravityScale = 0.28f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackDamage = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackCooldown = 0.35f;

    /** Au-dessous de cette altitude, on considere que le joueur est tombe du monde. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal", meta = (ClampMax = "0.0"))
    float FallResetZ = -900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
    bool bAutoResetOnFall = true;

protected:
    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void AttackPressed();
    void GlidePressed();
    void GlideReleased();

private:
    bool bIsAttacking = false;
    bool bIsGliding = false;
    float LastAttackTime = -100.0f;
    float SafeSpotTime = 0.0f;
    float CameraKickTime = 0.0f;
    bool bHasSafeSpot = false;
    FVector LastSafeSpot = FVector::ZeroVector;
};

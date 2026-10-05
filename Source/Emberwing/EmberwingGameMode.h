#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EmberwingGameMode.generated.h"

/**
 * GameMode du prototype : il monte l'arene ET l'eclairage avant que le premier joueur ne
 * soit possede, puis garantit un PlayerStart exploitable.
 *
 * C'est ici que part la logique anti-ecran-noir : lumieres Movable forcees, exposition figee,
 * brouillard a densite raisonnable, PlayerStart trace au sol.
 */
UCLASS()
class EMBERWING_API AEmberwingGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AEmberwingGameMode();

    /** Genere l'arene procedurale si le niveau n'en contient pas encore. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing")
    bool bBuildPrototypeWorld = true;

    /** Installe le rig de lumiere (a laisser true tant que le niveau n'est pas aute). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emberwing")
    bool bInstallLighting = true;

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void StartPlay() override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual bool ShouldSpawnAtStartSpot(AController* Player) override { return false; }

private:
    void EnsurePrototypeWorldAndPlayerStart();
    bool HasBlockingGroundBelow(const FVector& Location) const;

    /** Secours : si le pawn est reste sans vue pendant X secondes, on re-accroche la camera. */
    float CameraGuardTime = 0.0f;
};

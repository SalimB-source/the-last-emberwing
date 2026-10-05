#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EmberwingGameMode.generated.h"

UCLASS()
class EMBERWING_API AEmberwingGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AEmberwingGameMode();

protected:
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void StartPlay() override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual bool ShouldSpawnAtStartSpot(AController* Player) override { return false; }

private:
    void EnsurePrototypeWorldAndPlayerStart();
    void EnsureWorldLighting();
    FVector GetSafePlayerStartLocation() const;
};

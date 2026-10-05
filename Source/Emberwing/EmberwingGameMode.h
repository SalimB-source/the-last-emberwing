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
    virtual void StartPlay() override;
};

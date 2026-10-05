#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EmberwingPlayerController.generated.h"

UCLASS()
class EMBERWING_API AEmberwingPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AEmberwingPlayerController();

protected:
    virtual void BeginPlay() override;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmberwingPrototypeWorld.generated.h"

class UStaticMesh;
class USceneComponent;

UCLASS()
class EMBERWING_API AEmberwingPrototypeWorld : public AActor
{
    GENERATED_BODY()

public:
    AEmberwingPrototypeWorld();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    USceneComponent* SceneRoot;

    UPROPERTY()
    UStaticMesh* CubeMesh;

    UPROPERTY()
    UStaticMesh* SphereMesh;

    void AddPlatform(const FVector& Location, const FVector& Scale);
    void AddLantern(const FVector& Location);
    void SpawnEnemy(const FVector& Location);
};

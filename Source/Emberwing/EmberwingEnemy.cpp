#include "EmberwingEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AEmberwingEnemy::AEmberwingEnemy()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 52.0f);
    GetMesh()->SetVisibility(false);

    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->MaxWalkSpeed = 180.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 900.0f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
        PlaceholderBody->SetupAttachment(RootComponent);
        PlaceholderBody->SetStaticMesh(CubeMesh.Object);
        PlaceholderBody->SetRelativeLocation(FVector(0.0f, 0.0f, 48.0f));
        PlaceholderBody->SetRelativeScale3D(FVector(0.54f, 0.54f, 0.54f));
        PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void AEmberwingEnemy::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
}

void AEmberwingEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (!Player)
    {
        return;
    }

    const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
    const float Distance = ToPlayer.Size();
    if (Distance > 1100.0f || Distance < KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector Direction = ToPlayer.GetSafeNormal2D();
    if (Distance > 155.0f)
    {
        AddMovementInput(Direction, 0.42f);
        const FRotator DesiredRotation = Direction.Rotation();
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, 5.0f));
    }
    else if (GetWorld()->GetTimeSeconds() >= NextAttackTime)
    {
        NextAttackTime = GetWorld()->GetTimeSeconds() + 1.2f;
        UGameplayStatics::ApplyDamage(Player, ContactDamage, GetController(), this, UDamageType::StaticClass());
    }
}

float AEmberwingEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    CurrentHealth -= AppliedDamage;

    if (CurrentHealth <= 0.0f)
    {
        Destroy();
    }

    return AppliedDamage;
}

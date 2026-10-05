#include "EmberwingEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "EmberwingMantisRig.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AEmberwingEnemy::AEmberwingEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 52.0f);
	GetMesh()->SetVisibility(false, false);

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = 180.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 900.0f;
	GetCharacterMovement()->AirControl = 0.10f;

	StriderRig = CreateDefaultSubobject<UEmberwingMantisRig>(TEXT("StriderRig"));
	StriderRig->Style = EEmberwingModelStyle::DrownedStrider;
	StriderRig->ShellColor = FLinearColor(0.055f, 0.045f, 0.105f);
	StriderRig->BellyColor = FLinearColor(0.105f, 0.075f, 0.165f);
	StriderRig->BladeColor = FLinearColor(0.55f, 0.62f, 0.66f);
	StriderRig->EyeColor = FLinearColor(0.30f, 0.95f, 1.00f);
	StriderRig->WingColor = FLinearColor(0.20f, 0.30f, 0.45f);
	StriderRig->EmberLightIntensity = 1400.0f;

	// Sac d'air bioluminescent : le meme genre de lumiere portee que la braise du joueur,
	// pour que l'ennemi reste lisible meme si l'arene est mal eclairee.
	RotGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("RotGlow"));
	RotGlow->SetupAttachment(GetMesh());
	RotGlow->SetRelativeLocation(FVector(-12.0f, 0.0f, 66.0f));
	RotGlow->SetMobility(EComponentMobility::Movable);
	RotGlow->CastShadows = 0;
}

void AEmberwingEnemy::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (StriderRig)
	{
		StriderRig->SetEmberLight(RotGlow);
	}
}

void AEmberwingEnemy::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (StriderRig && !StriderRig->Build(GetMesh()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Emberwing] modele du patineur noye non construit (BasicShapes absents ?)"));
	}

	if (RotGlow)
	{
		RotGlow->SetLightColor(FLinearColor(0.30f, 0.85f, 1.00f), false);
		RotGlow->SetSourceRadius(14.0f);
		RotGlow->SetSoftSourceRadius(28.0f);
		RotGlow->SetUseInverseSquaredFalloff(true);
	}
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

	if (Distance > AggroRadius || Distance < KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Ne reste pas colle au joueur au bord d'une plateforme.
	const bool bPlayerIsReachable = FMath::Abs(ToPlayer.Z) < 320.0f;
	if (!bPlayerIsReachable && Distance > 240.0f)
	{
		return;
	}

	const FVector Direction = ToPlayer.GetSafeNormal2D();
	if (Distance > DamageRange)
	{
		AddMovementInput(Direction, 0.42f);
		const FRotator DesiredRotation = Direction.Rotation();
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, 5.0f));
	}
	else if (GetWorld()->GetTimeSeconds() >= NextAttackTime)
	{
		NextAttackTime = GetWorld()->GetTimeSeconds() + 1.2f;
		UGameplayStatics::ApplyDamage(Player, ContactDamage, GetController(), this, UDamageType::StaticClass());

		// Le patineur se redresse en piquant : meme signal visuel que l'attaque du joueur.
		if (StriderRig)
		{
			StriderRig->PlayAttack();
		}
	}
}

float AEmberwingEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth -= AppliedDamage;

	if (CurrentHealth <= 0.0f)
	{
		Destroy();
		return AppliedDamage;
	}

	// Feedback de hit : flash du sac d'air + petit recul, sans casser le chase.
	if (StriderRig)
	{
		StriderRig->FlashHit();
	}

	if (DamageCauser)
	{
		const FVector Knockback = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal() * 240.0f;
		LaunchCharacter(FVector(Knockback.X, Knockback.Y, 190.0f), false, true);
	}

	return AppliedDamage;
}

#include "EmberwingCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AEmberwingCharacter::AEmberwingCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);
    GetMesh()->SetVisibility(false);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
    Movement->JumpZVelocity = 620.0f;
    Movement->AirControl = 0.78f;
    Movement->MaxWalkSpeed = 480.0f;
    Movement->BrakingDecelerationWalking = 1800.0f;
    JumpMaxCount = 1;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 520.0f;
    CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
    CameraBoom->SetRelativeRotation(FRotator(-12.0f, 0.0f, 0.0f));
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
    FollowCamera->FieldOfView = 72.0f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
        PlaceholderBody->SetupAttachment(RootComponent);
        PlaceholderBody->SetStaticMesh(CubeMesh.Object);
        PlaceholderBody->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
        PlaceholderBody->SetRelativeScale3D(FVector(0.48f, 0.28f, 0.62f));
        PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        PlaceholderBlade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBlade"));
        PlaceholderBlade->SetupAttachment(RootComponent);
        PlaceholderBlade->SetStaticMesh(CubeMesh.Object);
        PlaceholderBlade->SetRelativeLocation(FVector(48.0f, 0.0f, 78.0f));
        PlaceholderBlade->SetRelativeRotation(FRotator(0.0f, 0.0f, -18.0f));
        PlaceholderBlade->SetRelativeScale3D(FVector(0.62f, 0.08f, 0.08f));
        PlaceholderBlade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void AEmberwingCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AEmberwingCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bIsGliding && GetCharacterMovement()->IsFalling())
    {
        GetCharacterMovement()->GravityScale = GlideGravityScale;
        GetCharacterMovement()->Velocity.Z = FMath::Max(GetCharacterMovement()->Velocity.Z, -180.0f);
    }
    else if (!bIsGliding)
    {
        GetCharacterMovement()->GravityScale = 1.0f;
    }
}

void AEmberwingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AEmberwingCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AEmberwingCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AEmberwingCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AEmberwingCharacter::LookUp);

    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &AEmberwingCharacter::AttackPressed);
    PlayerInputComponent->BindAction(TEXT("Glide"), IE_Pressed, this, &AEmberwingCharacter::GlidePressed);
    PlayerInputComponent->BindAction(TEXT("Glide"), IE_Released, this, &AEmberwingCharacter::GlideReleased);
}

void AEmberwingCharacter::MoveForward(float Value)
{
    if (Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
    }
}

void AEmberwingCharacter::MoveRight(float Value)
{
    if (Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
    }
}

void AEmberwingCharacter::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void AEmberwingCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

void AEmberwingCharacter::AttackPressed()
{
    PerformLightAttack();
}

void AEmberwingCharacter::PerformLightAttack()
{
    const UWorld* World = GetWorld();
    if (!World || bIsAttacking || World->GetTimeSeconds() - LastAttackTime < AttackCooldown)
    {
        return;
    }

    LastAttackTime = World->GetTimeSeconds();
    bIsAttacking = true;

    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 58.0f) + GetActorForwardVector() * 35.0f;
    const FVector End = Start + GetActorForwardVector() * 180.0f;
    const FCollisionShape AttackShape = FCollisionShape::MakeSphere(72.0f);
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EmberwingLightAttack), false, this);

    TArray<FHitResult> Hits;
    if (World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, AttackShape, QueryParams))
    {
        TSet<AActor*> AlreadyHit;
        for (const FHitResult& Hit : Hits)
        {
            AActor* HitActor = Hit.GetActor();
            if (HitActor && !AlreadyHit.Contains(HitActor))
            {
                AlreadyHit.Add(HitActor);
                UGameplayStatics::ApplyDamage(HitActor, AttackDamage, GetController(), this, UDamageType::StaticClass());
            }
        }
    }

    FTimerHandle AttackTimer;
    GetWorldTimerManager().SetTimer(AttackTimer, [this]() { bIsAttacking = false; }, 0.22f, false);
}

void AEmberwingCharacter::GlidePressed()
{
    StartGlide();
}

void AEmberwingCharacter::GlideReleased()
{
    StopGlide();
}

void AEmberwingCharacter::StartGlide()
{
    bIsGliding = true;
}

void AEmberwingCharacter::StopGlide()
{
    bIsGliding = false;
    GetCharacterMovement()->GravityScale = 1.0f;
}

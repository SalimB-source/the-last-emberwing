#include "EmberwingCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EmberwingMantisRig.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

AEmberwingCharacter::AEmberwingCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);

    // Le Mesh reste cache : c'est sous lui que le rig procedural est monte (le Mesh est
    // deja decale de -HalfHeight, donc le (0,0,0) local = la plante des pieds).
    GetMesh()->SetVisibility(false, false);

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

    // --- Camera troisieme personne ------------------------------------------------------
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 520.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 82.0f);
    CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 46.0f);
    CameraBoom->bUsePawnControlRotation = true;
    // bDoCollisionTest VOLONTAIREMENT desactive : avec le BSP du niveau prototype, le probe
    // de butait sur la geometrie et ecrasait la camera DANS le crane du personnage -> noir.
    CameraBoom->bDoCollisionTest = false;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 14.0f;
    CameraBoom->bEnableCameraRotationLag = true;
    CameraBoom->CameraRotationLagSpeed = 20.0f;
    CameraBoom->bInheritRoll = false;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
    FollowCamera->FieldOfView = 82.0f;
    FollowCamera->bAutoActivate = true;

    // PAS d'override d'exposition ici : la camera est appliquee APRES les volumes, un
    // reglage ici masquerait le PostProcessVolume du rig (et son curseur ExposureBrightness).

    // --- Braise portee ------------------------------------------------------------------
    // Creee dans le constructeur pour que la mobilite soit Movable avant enregistrement :
    // c'est la seule facon fiable d'avoir une lumiere qui fonctionne sans bake.
    EmberLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EmberLight"));
    EmberLight->SetupAttachment(GetMesh());
    EmberLight->SetRelativeLocation(FVector(6.0f, 0.0f, 92.0f));
    EmberLight->SetMobility(EComponentMobility::Movable);
    EmberLight->CastShadows = 0;
    EmberLight->bAffectsWorld = 1;
    EmberLight->SetLightColor(FLinearColor(1.0f, 0.45f, 0.12f), false);
    EmberLight->SetSourceRadius(16.0f);
    EmberLight->SetSoftSourceRadius(34.0f);
    EmberLight->SetUseInverseSquaredFalloff(true);
    // L'intensite, elle, est pilote par le rig (pulsation + flash + boost d'attaque).

    MantisRig = CreateDefaultSubobject<UEmberwingMantisRig>(TEXT("MantisRig"));
}

void AEmberwingCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (MantisRig)
    {
        MantisRig->SetEmberLight(EmberLight);
    }
}

void AEmberwingCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Modele procedural + lumiere de la braise.
    if (MantisRig && !MantisRig->Build(GetMesh()))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Emberwing] le modele procedural du personnage n'a pas pu etre construit (BasicShapes absents ?)"));
    }

    // Regarde legerement vers l'horizon : un spawn pitch = -90 fixe donne un ciel/sol noir.
    if (Controller)
    {
        FRotator StartRotation = GetActorRotation();
        StartRotation.Pitch = -10.0f;
        Controller->SetControlRotation(StartRotation);
    }

    EnsureCameraActive();
}

void AEmberwingCharacter::EnsureCameraActive()
{
    if (FollowCamera)
    {
        FollowCamera->Activate();
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bAutoManageActiveCameraTarget = true;
        PC->SetViewTargetWithBlend(this, 0.0f);
    }
}

void AEmberwingCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UCharacterMovementComponent* Movement = GetCharacterMovement();

    // --- Plane -------------------------------------------------------------------------
    if (bIsGliding && Movement && Movement->IsFalling())
    {
        Movement->GravityScale = GlideGravityScale;
        Movement->Velocity.Z = FMath::Max(Movement->Velocity.Z, -190.0f);
    }
    else if (Movement && !bIsGliding)
    {
        Movement->GravityScale = 1.0f;
    }

    // --- Memoire des appuis + piege a vide ------------------------------------------------
    if (Movement && Movement->IsMovingOnGround())
    {
        SafeSpotTime += DeltaSeconds;
        if (SafeSpotTime > 0.2f)
        {
            SafeSpotTime = 0.0f;
            LastSafeSpot = GetActorLocation();
            bHasSafeSpot = true;
        }
    }

    if (bAutoResetOnFall && GetActorLocation().Z < FallResetZ)
    {
        ResetFall();
    }

    // --- Coup de FOV a l'attaque (game feel) --------------------------------------------
    if (CameraKickTime > 0.0f)
    {
        CameraKickTime = FMath::Max(0.0f, CameraKickTime - DeltaSeconds);
        if (FollowCamera)
        {
            const float K = CameraKickTime / 0.18f;
            FollowCamera->FieldOfView = 82.0f + FMath::Sin(K * PI) * 5.0f;
        }
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
        const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
    }
}

void AEmberwingCharacter::MoveRight(float Value)
{
    if (Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
    {
        const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
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
    CameraKickTime = 0.18f;

    if (MantisRig)
    {
        MantisRig->PlayAttack();
    }

    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 58.0f) + GetActorForwardVector() * 35.0f;
    const FVector End = Start + GetActorForwardVector() * 180.0f;
    const FCollisionShape AttackShape = FCollisionShape::MakeSphere(72.0f);
    // Constructeur par defaut + setters : seule forme sure d'un bout a l'autre des versions
    // 5.x (le 3e argument du constructeur a change de type entre les versions).
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    QueryParams.AddIgnoredActor(this);

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

    // L'animation est un peu plus longue que le hitbox : l'armede rattrape visuellement.
    FTimerHandle AttackTimer;
    GetWorldTimerManager().SetTimer(AttackTimer, [this]()
    {
        bIsAttacking = false;
    }, 0.30f, false);
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

    if (MantisRig)
    {
        MantisRig->SetGlide(true);
    }
}

void AEmberwingCharacter::StopGlide()
{
    bIsGliding = false;

    if (MantisRig)
    {
        MantisRig->SetGlide(false);
    }

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->GravityScale = 1.0f;
    }
}

void AEmberwingCharacter::ResetFall()
{
    const FVector Target = bHasSafeSpot ? LastSafeSpot : FVector(700.0f, 0.0f, 160.0f);

    StopGlide();
    SetActorLocation(Target + FVector(0.0f, 0.0f, 40.0f), false, nullptr, ETeleportType::ResetPhysics);

    if (Controller)
    {
        Controller->SetControlRotation(FRotator(-10.0f, 0.0f, 0.0f));
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(91200, 2.5f, FColor::Orange, TEXT("Emberwing - chute dans le vide, retour au dernier appui"));
    }
}

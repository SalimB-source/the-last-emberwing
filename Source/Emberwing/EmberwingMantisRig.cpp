#include "EmberwingMantisRig.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EmberwingProceduralAssets.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// Toutes les valeurs ci-dessous sont en centimetres (unites moteur) et partent de la
// PLANTE DES PIEDS : le rig est attache au Mesh du Character, qui est deja decale de
// -HalfHeight. La capsule fait 88 de demi-hauteur => un corps d'environ 170 de haut.

UEmberwingMantisRig::UEmberwingMantisRig()
{
	PrimaryComponentTick.bCanEverTick = true;
}

USceneComponent* UEmberwingMantisRig::AddJoint(USceneComponent* Parent, const FVector& Location, const FRotator& Rotation)
{
	AActor* Owner = GetOwner();
	if (!Parent || !Owner)
	{
		return nullptr;
	}

	USceneComponent* Joint = NewObject<USceneComponent>(Owner);
	Owner->AddInstanceComponent(Joint);
	Joint->SetupAttachment(Parent);
	Joint->SetRelativeLocation(Location);
	Joint->SetRelativeRotation(Rotation);
	Joint->RegisterComponent();

	Joints.Add(Joint);
	return Joint;
}

UStaticMeshComponent* UEmberwingMantisRig::AddPart(USceneComponent* Parent, EEmberwingPrimitive Primitive, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color, float EmissiveStrength, bool bCastShadow)
{
	AActor* Owner = GetOwner();
	UStaticMesh* Mesh = PrimitiveMesh(Primitive);

	if (!Parent || !Owner || !Mesh)
	{
		return nullptr;
	}

	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner);
	Owner->AddInstanceComponent(Part);
	Part->SetStaticMesh(Mesh);
	Part->SetupAttachment(Parent);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->CastShadow = bCastShadow ? 1 : 0;
	Part->bCastDynamicShadow = bCastShadow ? 1 : 0;

	FEmberwingAssets::ApplyTint(Part, Color, EmissiveStrength);
	Part->RegisterComponent();

	Parts.Add(Part);
	return Part;
}

UStaticMesh* UEmberwingMantisRig::PrimitiveMesh(EEmberwingPrimitive Primitive) const
{
	return FEmberwingAssets::GetPrimitive(Primitive);
}

bool UEmberwingMantisRig::Build(USceneComponent* Parent)
{
	if (bBuilt)
	{
		return true;
	}

	if (!Parent)
	{
		return false;
	}

	RootJoint = AddJoint(Parent, FVector::ZeroVector, FRotator::ZeroRotator);
	if (!RootJoint)
	{
		return false;
	}

	if (Style == EEmberwingModelStyle::DrownedStrider)
	{
		BuildStrider();
	}
	else
	{
		BuildMantis();
	}

	// Un rig sans aucune maille visible doit etre detectable (diagnostic a l'ecran).
	bBuilt = (Parts.Num() > 0);
	return bBuilt;
}

void UEmberwingMantisRig::BuildMantis()
{
	// --- Bassin + thorax + abdomen segmente -------------------------------------------
	Pelvis = AddJoint(RootJoint, FVector(0.0f, 0.0f, 64.0f), FRotator::ZeroRotator);
	Thorax = AddJoint(Pelvis, FVector(0.0f, 0.0f, 26.0f), FRotator::ZeroRotator);
	AddPart(Thorax, EEmberwingPrimitive::Cube, FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f), FVector(0.46f, 0.40f, 0.30f), ShellColor, 0.0f, true);
	AddPart(Thorax, EEmberwingPrimitive::Cube, FVector(4.0f, 0.0f, 15.0f), FRotator(-8.0f, 0.0f, 0.0f), FVector(0.34f, 0.32f, 0.10f), ShellColor, 0.0f, true);

	AbdomenRoot = AddJoint(Pelvis, FVector(-18.0f, 0.0f, 4.0f), FRotator::ZeroRotator);
	AddPart(AbdomenRoot, EEmberwingPrimitive::Cube, FVector(-16.0f, 0.0f, 0.0f), FRotator::ZeroRotator, FVector(0.34f, 0.28f, 0.26f), BellyColor, 0.0f, true);
	AddPart(AbdomenRoot, EEmberwingPrimitive::Cube, FVector(-42.0f, 0.0f, -3.0f), FRotator(0.0f, 0.0f, 0.0f), FVector(0.26f, 0.21f, 0.20f), BellyColor, 0.0f, true);
	AddPart(AbdomenRoot, EEmberwingPrimitive::Cube, FVector(-62.0f, 0.0f, -8.0f), FRotator(0.0f, 0.0f, 0.0f), FVector(0.16f, 0.14f, 0.14f), BellyColor, 0.0f, true);

	// --- Tete, yeux, antennes ---------------------------------------------------------
	Neck = AddJoint(Thorax, FVector(4.0f, 0.0f, 22.0f), FRotator::ZeroRotator);
	HeadMesh = AddPart(Neck, EEmberwingPrimitive::Cube, FVector(0.0f, 0.0f, 4.0f), FRotator(0.0f, 0.0f, 0.0f), FVector(0.26f, 0.36f, 0.22f), ShellColor, 0.0f, true);
	AddPart(Neck, EEmberwingPrimitive::Cube, FVector(13.0f, 0.0f, -2.0f), FRotator(0.0f, 0.0f, 0.0f), FVector(0.10f, 0.24f, 0.10f), ShellColor, 0.0f, true);
	AddPart(Neck, EEmberwingPrimitive::Cone, FVector(-4.0f, 0.0f, 15.0f), FRotator(0.0f, 0.0f, -90.0f), FVector(0.18f, 0.30f, 0.30f), ShellColor, 0.0f, true);

	// Les yeux sont emisssifs : ils restent lisibles meme si l'arene est sous-eclairee.
	AddPart(Neck, EEmberwingPrimitive::Sphere, FVector(9.0f, -15.0f, 6.0f), FRotator::ZeroRotator, FVector(0.15f), EyeColor, 6.0f, false);
	AddPart(Neck, EEmberwingPrimitive::Sphere, FVector(9.0f, 15.0f, 6.0f), FRotator::ZeroRotator, FVector(0.15f), EyeColor, 6.0f, false);
	AddPart(Neck, EEmberwingPrimitive::Cube, FVector(16.0f, -7.0f, 14.0f), FRotator(24.0f, -18.0f, 0.0f), FVector(0.52f, 0.025f, 0.025f), ShellColor, 0.0f, false);
	AddPart(Neck, EEmberwingPrimitive::Cube, FVector(16.0f, 7.0f, 14.0f), FRotator(24.0f, 18.0f, 0.0f), FVector(0.52f, 0.025f, 0.025f), ShellColor, 0.0f, false);

	// --- 6 pattes (trio de droite / trio de gauche, allure tripodiale) ----------------
	// Hanche legerement au-dessus du thorax, pattes en trio (avant / milieu / arriere).
	const float HipZ = -4.0f;
	AddLeg(Thorax, FVector(6.0f, -16.0f, HipZ), 62.0f, -1.0f, 0.0f, 1.00f, -4.0f, -68.0f);
	AddLeg(Thorax, FVector(-12.0f, -17.0f, HipZ), 108.0f, -1.0f, 2.094f, 1.05f, -4.0f, -70.0f);
	AddLeg(Thorax, FVector(-30.0f, -16.0f, HipZ), 152.0f, -1.0f, 4.188f, 0.95f, -6.0f, -72.0f);
	AddLeg(Thorax, FVector(6.0f, 16.0f, HipZ), 62.0f, 1.0f, 3.1416f, 1.00f, -4.0f, -68.0f);
	AddLeg(Thorax, FVector(-12.0f, 17.0f, HipZ), 108.0f, 1.0f, 5.236f, 1.05f, -4.0f, -70.0f);
	AddLeg(Thorax, FVector(-30.0f, 16.0f, HipZ), 152.0f, 1.0f, 1.047f, 0.95f, -6.0f, -72.0f);

	BuildArms(Thorax);
	BuildWings(Thorax);

	// --- Coeur d'ember : la "derniere braise" du titre -------------------------------
	EmberCore = AddPart(Thorax, EEmberwingPrimitive::Sphere, FVector(12.0f, 0.0f, -6.0f), FRotator::ZeroRotator, FVector(0.15f), EyeColor, 14.0f, false);
}

void UEmberwingMantisRig::BuildStrider()
{
	// Ennemi du Drowned Root : corps effile, pattes-becs tres longues, 4 membres.
	Pelvis = AddJoint(RootJoint, FVector(0.0f, 0.0f, 78.0f), FRotator::ZeroRotator);
	Thorax = AddJoint(Pelvis, FVector(0.0f, 0.0f, 12.0f), FRotator::ZeroRotator);
	AddPart(Thorax, EEmberwingPrimitive::Sphere, FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator, FVector(0.34f, 0.30f, 0.26f), ShellColor, 0.0f, true);

	AbdomenRoot = AddJoint(Pelvis, FVector(-16.0f, 0.0f, 2.0f), FRotator::ZeroRotator);
	AddPart(AbdomenRoot, EEmberwingPrimitive::Sphere, FVector(-18.0f, 0.0f, -4.0f), FRotator::ZeroRotator, FVector(0.34f, 0.24f, 0.24f), BellyColor, 0.0f, true);
	AddPart(AbdomenRoot, EEmberwingPrimitive::Cone, FVector(-44.0f, 0.0f, -8.0f), FRotator(0.0f, 0.0f, 90.0f), FVector(0.30f, 0.16f, 0.16f), BellyColor, 0.0f, true);

	Neck = AddJoint(Thorax, FVector(6.0f, 0.0f, 16.0f), FRotator::ZeroRotator);
	HeadMesh = AddPart(Neck, EEmberwingPrimitive::Cone, FVector(6.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, -90.0f), FVector(0.30f, 0.20f, 0.20f), ShellColor, 0.0f, true);
	AddPart(Neck, EEmberwingPrimitive::Sphere, FVector(2.0f, -11.0f, 4.0f), FRotator::ZeroRotator, FVector(0.13f), EyeColor, 7.0f, false);
	AddPart(Neck, EEmberwingPrimitive::Sphere, FVector(2.0f, 11.0f, 4.0f), FRotator::ZeroRotator, FVector(0.13f), EyeColor, 7.0f, false);

	// Echasses : la hanche est a z ~ 78, les tibias restent tres allonges et peu plonges.
	AddLeg(Thorax, FVector(10.0f, -13.0f, -12.0f), 52.0f, -1.0f, 0.0f, 1.15f, -6.0f, -40.0f);
	AddLeg(Thorax, FVector(-6.0f, -14.0f, -12.0f), 128.0f, -1.0f, 2.6f, 1.20f, -6.0f, -38.0f);
	AddLeg(Thorax, FVector(10.0f, 13.0f, -12.0f), 52.0f, 1.0f, 3.1416f, 1.15f, -6.0f, -40.0f);
	AddLeg(Thorax, FVector(-6.0f, 14.0f, -12.0f), 128.0f, 1.0f, 0.55f, 1.20f, -6.0f, -38.0f);
}

void UEmberwingMantisRig::AddLeg(USceneComponent* Parent, const FVector& HipLocation, float YawDegrees, float SideSign, float Phase, float LengthScale, float HipPitch, float KneePitch)
{
	if (!Parent)
	{
		return;
	}

	// Rotation au repos : la hanche pointe vers l'exterieur (yaw especulaire selon le cote),
	// le genou plombe vers le bas. Les 2 angles sont stockes dans la patte car AnimateLegs()
	// reecrit la rotation complete : ils servent aussi a poser la patte au sol (z ~ 0).
	const float MirroredYaw = YawDegrees * SideSign;
	USceneComponent* Hip = AddJoint(Parent, HipLocation, FRotator(HipPitch, MirroredYaw, 0.0f));
	if (!Hip)
	{
		return;
	}

	const float ThighLength = 30.0f * LengthScale;
	const float ShinLength = 95.0f * LengthScale;

	UStaticMeshComponent* Thigh = AddPart(Hip, EEmberwingPrimitive::Cube, FVector(ThighLength * 0.5f, 0.0f, 0.0f), FRotator::ZeroRotator,
		FVector(ThighLength * 0.01f, 0.075f, 0.075f), ShellColor, 0.0f, true);

	USceneComponent* Knee = AddJoint(Hip, FVector(ThighLength, 0.0f, 0.0f), FRotator(KneePitch, 0.0f, 0.0f));
	UStaticMeshComponent* Shin = nullptr;

	if (Knee)
	{
		Shin = AddPart(Knee, EEmberwingPrimitive::Cube, FVector(ShinLength * 0.5f, 0.0f, 0.0f), FRotator::ZeroRotator,
			FVector(ShinLength * 0.01f, 0.05f, 0.05f), ShellColor, 0.0f, true);
		// Griffe : petit segment couche au bout du tibia.
		AddPart(Knee, EEmberwingPrimitive::Cube, FVector(ShinLength, 0.0f, -3.0f), FRotator(0.0f, 0.0f, 90.0f),
			FVector(0.16f, 0.10f, 0.05f), BladeColor, 0.0f, false);
	}

	FEmberwingLeg Leg;
	Leg.Hip = Hip;
	Leg.Knee = Knee;
	Leg.Thigh = Thigh;
	Leg.Shin = Shin;
	Leg.Phase = Phase;
	Leg.SideSign = SideSign;
	Leg.RestYaw = MirroredYaw;
	Leg.RestPitch = HipPitch;
	Leg.RestKneePitch = KneePitch;
	Legs.Add(Leg);
}

void UEmberwingMantisRig::BuildArms(USceneComponent* Parent)
{
	if (!Parent)
	{
		return;
	}

	// Poses de repos. AnimateArms() reecrit la rotation complete des articulations a partir
	// des MEMES valeurs : si on change l'un, il faut changer l'autre.
	const float RestShoulderPitch = 14.0f;
	const float RestShoulderYaw = 16.0f;
	const float RestShoulderRoll = -8.0f;
	const float RestElbowPitch = 46.0f;
	const float RestElbowYaw = -10.0f;
	const float RestElbowRoll = 26.0f;

	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float SideSign = (Side == 0) ? -1.0f : 1.0f;

		USceneComponent* Shoulder = AddJoint(Parent,
			FVector(10.0f, SideSign * 13.0f, 8.0f),
			FRotator(RestShoulderPitch, SideSign * RestShoulderYaw, SideSign * RestShoulderRoll));
		if (!Shoulder)
		{
			continue;
		}

		// Coxa
		AddPart(Shoulder, EEmberwingPrimitive::Cube, FVector(11.0f, 0.0f, 0.0f), FRotator::ZeroRotator,
			FVector(0.24f, 0.09f, 0.09f), ShellColor, 0.0f, true);

		USceneComponent* Elbow = AddJoint(Shoulder, FVector(24.0f, 0.0f, -2.0f),
			FRotator(RestElbowPitch, SideSign * RestElbowYaw, SideSign * RestElbowRoll));

		FEmberwingArm Arm;
		Arm.Shoulder = Shoulder;
		Arm.Elbow = Elbow;
		Arm.SideSign = SideSign;

		if (Elbow)
		{
			// Femur (bras raptorial) + tibia/faucille
			Arm.ForeArm = AddPart(Elbow, EEmberwingPrimitive::Cube, FVector(19.0f, 0.0f, 0.0f), FRotator::ZeroRotator,
				FVector(0.40f, 0.11f, 0.13f), ShellColor, 0.0f, true);
			Arm.Blade = AddPart(Elbow, EEmberwingPrimitive::Cube, FVector(52.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f),
				FVector(0.30f, 0.03f, 0.16f), BladeColor, 0.0f, true);
			AddPart(Elbow, EEmberwingPrimitive::Cone, FVector(70.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, -90.0f),
				FVector(0.16f, 0.05f, 0.10f), BladeColor, 0.0f, false);

			// Epines du bras raptorial
			AddPart(Elbow, EEmberwingPrimitive::Cube, FVector(30.0f, 0.0f, 8.0f), FRotator(0.0f, 0.0f, 90.0f),
				FVector(0.09f, 0.03f, 0.03f), BladeColor, 0.0f, false);
			AddPart(Elbow, EEmberwingPrimitive::Cube, FVector(42.0f, 0.0f, 8.0f), FRotator(0.0f, 0.0f, 90.0f),
				FVector(0.07f, 0.03f, 0.03f), BladeColor, 0.0f, false);
		}

		Arms.Add(Arm);
	}
}

void UEmberwingMantisRig::BuildWings(USceneComponent* Parent)
{
	if (!Parent)
	{
		return;
	}

	WingLeftJoint = AddJoint(Parent, FVector(-4.0f, -12.0f, 16.0f), FRotator::ZeroRotator);
	WingRightJoint = AddJoint(Parent, FVector(-4.0f, 12.0f, 16.0f), FRotator::ZeroRotator);

	WingLeft = AddPart(WingLeftJoint, EEmberwingPrimitive::Cube, FVector(-6.0f, -34.0f, 0.0f), FRotator::ZeroRotator,
		FVector(0.52f, 0.68f, 0.018f), WingColor, 0.6f, true);
	WingRight = AddPart(WingRightJoint, EEmberwingPrimitive::Cube, FVector(-6.0f, 34.0f, 0.0f), FRotator::ZeroRotator,
		FVector(0.52f, 0.68f, 0.018f), WingColor, 0.6f, true);

	// Nervures : deux cubes fins donnent une lecture "aile d'insecte" au lieu d'une plaque.
	AddPart(WingLeftJoint, EEmberwingPrimitive::Cube, FVector(-6.0f, -34.0f, 2.0f), FRotator(0.0f, 0.0f, 18.0f),
		FVector(0.50f, 0.02f, 0.02f), ShellColor, 0.0f, false);
	AddPart(WingRightJoint, EEmberwingPrimitive::Cube, FVector(-6.0f, 34.0f, 2.0f), FRotator(0.0f, 0.0f, -18.0f),
		FVector(0.50f, 0.02f, 0.02f), ShellColor, 0.0f, false);
}

void UEmberwingMantisRig::PlayAttack()
{
	AttackTime = AttackDuration;
}

void UEmberwingMantisRig::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bBuilt || !GetOwner())
	{
		return;
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;

	const float Speed = Character ? Character->GetVelocity().Size2D() : 0.0f;
	const float MaxSpeed = Movement ? FMath::Max(Movement->MaxWalkSpeed, 1.0f) : 480.0f;
	const bool bGrounded = Movement ? Movement->IsMovingOnGround() : true;
	const float SpeedRatio = FMath::Clamp(Speed / MaxSpeed, 0.0f, 1.4f);

	SmoothedSpeedRatio = FMath::FInterpTo(SmoothedSpeedRatio, SpeedRatio, DeltaTime, 12.0f);
	IdleTime += DeltaTime;

	if (AttackTime > 0.0f)
	{
		AttackTime = FMath::Max(0.0f, AttackTime - DeltaTime);
	}

	if (FlashRemaining > 0.0f)
	{
		FlashRemaining = FMath::Max(0.0f, FlashRemaining - DeltaTime);
		FlashPop = FMath::Sin((1.0f - (FlashRemaining / 0.18f)) * PI) * 0.06f;
	}
	else
	{
		FlashPop = FMath::FInterpTo(FlashPop, 0.0f, DeltaTime, 10.0f);
	}

	// Un insecte marque le pas bien plus vite qu'un humain : 2 enjambles par seconde au pas.
	GaitPhase += DeltaTime * (1.9f + 10.5f * SmoothedSpeedRatio);

	AnimateBody(DeltaTime, SmoothedSpeedRatio, bGrounded);
	AnimateLegs(DeltaTime, SmoothedSpeedRatio, bGrounded);
	AnimateArms(DeltaTime, SmoothedSpeedRatio);
	AnimateWings(DeltaTime, SmoothedSpeedRatio, bGrounded);

	if (EmberLight)
	{
		const float Pulse = 0.80f + 0.20f * FMath::Sin(IdleTime * 3.4f);
		const float AttackBoost = (AttackTime > 0.0f) ? 2.6f : 1.0f;
		// Flash a la prise de degats : gere ici, le proprietaire n'a pas a toucher la lumiere.
		const float HitFlash = 1.0f + 2.6f * FMath::Clamp(FlashRemaining / 0.18f, 0.0f, 1.0f);
		EmberLight->SetIntensity(EmberLightIntensity * Pulse * AttackBoost * HitFlash);
	}

	if (EmberCore)
	{
		const float CorePulse = 1.0f + 0.05f * FMath::Sin(IdleTime * 3.4f);
		EmberCore->SetRelativeScale3D(FVector(0.15f * CorePulse));
	}
}

void UEmberwingMantisRig::AnimateBody(float DeltaTime, float SpeedRatio, bool bGrounded)
{
	const float Breathe = FMath::Sin(IdleTime * 1.7f) * 0.014f * AnimationScale;
	const float Bob = FMath::Sin(GaitPhase * 2.0f) * 2.8f * AnimationScale * (0.30f + SpeedRatio);
	const float Lean = FMath::Clamp(SpeedRatio, 0.0f, 1.0f) * 10.0f + (bGrounded ? 0.0f : -8.0f);

	if (Pelvis)
	{
		Pelvis->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f + (Style == EEmberwingModelStyle::DrownedStrider ? 14.0f : 0.0f) + Bob));
	}

	if (Thorax)
	{
		// Pitch negatif = poitrine enfoncee vers l'avant (sens de rotation moteur).
		Thorax->SetRelativeRotation(FRotator(-Lean, FMath::Sin(GaitPhase) * 2.6f * AnimationScale, 0.0f));
		Thorax->SetRelativeScale3D(FVector(1.0f + Breathe + FlashPop, 1.0f - Breathe * 0.6f, 1.0f + Breathe));
	}

	if (AbdomenRoot)
	{
		AbdomenRoot->SetRelativeRotation(FRotator(
			Lean * 0.45f + FMath::Sin(IdleTime * 1.15f) * 2.4f * AnimationScale,
			FMath::Sin(GaitPhase) * 3.4f * AnimationScale,
			0.0f));
	}

	if (Neck)
	{
		Neck->SetRelativeRotation(FRotator(
			Lean * 0.55f - (bGrounded ? 0.0f : 6.0f),
			FMath::Sin(GaitPhase * 0.5f) * 4.5f * AnimationScale,
			0.0f));
	}
}

void UEmberwingMantisRig::AnimateLegs(float DeltaTime, float SpeedRatio, bool bGrounded)
{
	const float Amp = (0.22f + SpeedRatio) * AnimationScale * (bGrounded ? 1.0f : 0.40f);

	for (FEmberwingLeg& Leg : Legs)
	{
		if (!Leg.Hip)
		{
			continue;
		}

		const float Phase = GaitPhase + Leg.Phase;
		const float Swing = FMath::Sin(Phase);
		const float Lift = FMath::Max(0.0f, FMath::Cos(Phase));

		Leg.Hip->SetRelativeRotation(FRotator(
			Leg.RestPitch + Swing * 22.0f * Amp,
			Leg.RestYaw,
			Leg.SideSign * (Lift * 7.0f * Amp)));

		if (Leg.Knee)
		{
			// On repart TOUJOURS de la flexion de repos : l'ecrire en delta a chaque frame
			// ferait remonter toutes les pattes a l'horizontale des la premiere frame.
			const float Bend = FMath::Max(0.0f, -Swing) * 22.0f * Amp;
			Leg.Knee->SetRelativeRotation(FRotator(Leg.RestKneePitch - Bend, 0.0f, 0.0f));
		}
	}
}

void UEmberwingMantisRig::AnimateArms(float DeltaTime, float SpeedRatio)
{
	// Les constantes ci-dessous DOIVENT rester identiques a BuildArms() : l'animation
	// reecrit la rotation complete des articulations, jamais un delta.
	const float RestShoulderPitch = 14.0f;
	const float RestShoulderYaw = 16.0f;
	const float RestShoulderRoll = -8.0f;
	const float RestElbowPitch = 46.0f;
	const float RestElbowYaw = -10.0f;
	const float RestElbowRoll = 26.0f;

	const float Idle = FMath::Sin(IdleTime * 1.35f) * 3.0f * AnimationScale;
	const float Ready = FMath::Clamp(1.0f - SpeedRatio, 0.0f, 1.0f) * 6.0f;

	float ShoulderDelta = 0.0f;
	float BladeClose = 0.0f;

	if (AttackTime > 0.0f)
	{
		const float t = 1.0f - (AttackTime / FMath::Max(AttackDuration, 0.01f));

		if (t < 0.28f)
		{
			const float u = t / 0.28f;
			ShoulderDelta = -44.0f * u;
			BladeClose = 20.0f * u;
		}
		else if (t < 0.58f)
		{
			const float u = (t - 0.28f) / 0.30f;
			ShoulderDelta = FMath::Lerp(-44.0f, 38.0f, u);
			BladeClose = FMath::Lerp(20.0f, -74.0f, u);
		}
		else
		{
			const float u = (t - 0.58f) / 0.42f;
			ShoulderDelta = FMath::Lerp(38.0f, 0.0f, u);
			BladeClose = FMath::Lerp(-74.0f, 0.0f, u);
		}
	}

	for (FEmberwingArm& Arm : Arms)
	{
		const float SideSign = Arm.SideSign;

		if (Arm.Shoulder)
		{
			Arm.Shoulder->SetRelativeRotation(FRotator(
				RestShoulderPitch + ShoulderDelta * 0.60f + Idle,
				SideSign * (RestShoulderYaw + ShoulderDelta * 0.30f + Ready * 0.4f),
				SideSign * (RestShoulderRoll + ShoulderDelta * 0.20f)));
		}

		if (Arm.Elbow)
		{
			Arm.Elbow->SetRelativeRotation(FRotator(
				RestElbowPitch - BladeClose - Ready,
				SideSign * RestElbowYaw,
				SideSign * RestElbowRoll));
		}
	}
}

void UEmberwingMantisRig::AnimateWings(float DeltaTime, float SpeedRatio, bool bGrounded)
{
	if (!WingLeftJoint && !WingRightJoint)
	{
		return;
	}

	const float TargetSpread = bGlide ? 1.0f : 0.0f;
	WingSpread = FMath::FInterpTo(WingSpread, TargetSpread, DeltaTime, bGlide ? 9.5f : 5.5f);

	// Repliees le long de l'abdomen au sol, ouvertes et battantes en plane.
	const float Flap = bGlide
		? FMath::Sin(IdleTime * 10.5f) * 15.0f
		: FMath::Sin(GaitPhase * 2.0f) * 1.6f;

	const float FallPitch = bGrounded ? 0.0f : -10.0f;

	if (WingLeftJoint)
	{
		WingLeftJoint->SetRelativeRotation(FRotator(
			-6.0f - WingSpread * 14.0f + FallPitch,
			-10.0f,
			FMath::Lerp(5.0f, -56.0f, WingSpread) - Flap));
	}

	if (WingRightJoint)
	{
		WingRightJoint->SetRelativeRotation(FRotator(
			-6.0f - WingSpread * 14.0f + FallPitch,
			10.0f,
			FMath::Lerp(-5.0f, 56.0f, WingSpread) + Flap));
	}
}

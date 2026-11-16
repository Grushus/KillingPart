// Killing Part: the seamstress.

#include "KillingPart/KPHeroine.h"
#include "KillingPart/KPTypes.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace KPHeroineLook
{
	// Shears: pivot at her right hand, blades forward in a V, two round ring handles behind the pivot.
	// Only shown during the dash and the beat (Simon's T2 playtest: at rest they read badly).
	// T7d: tapered blades (flattened cones), a short shank and round rings of little cylinders, a screw at the pivot.
	const FVector ShearsOffset(38.f, 30.f, 96.f);
	constexpr float BladeLength = 96.f;
	constexpr float BladeWidth = 17.f;
	constexpr float BladeThick = 3.f;
	constexpr int32 RingSegments = 12;
	constexpr int32 PartsPerSide = 2 + RingSegments; // blade, shank, ring
	constexpr float RingRadius = 11.f;
	constexpr float RingTube = 2.6f;
	constexpr float RingBack = 19.f;   // ring centre behind the pivot
	constexpr float RingSide = 11.f;   // and to the side, so the two rings sit side by side
	constexpr float Scale = 1.4f;      // T7d: a bit bigger than life so she reads from the high camera (the hitbox stays heroRadius)

	// T7f: the Tripo model. In its own space Z is up, she faces +Y, her right is -X; 98 cm tall, feet at Z 0.
	// The T7d/T7e mannequin look stays in the code but isn't loaded.
	constexpr bool bMannequin = false;
	const TCHAR* const ModelPath = TEXT("/Game/KillingPart/Heroine/SM_Heroine.SM_Heroine");
	const FVector RightHand(-23.3f, 1.2f, 36.f);   // her right hand in the model's space (found from its vertices)
	const FVector ShearsFromHand(23.f, 8.f, 0.f);  // the pivot ahead of her fist (the rings in it), a little out from her skirt
	// Procedural life (real time). Angles in degrees, lengths in the actor's units (x Scale in the world).
	constexpr float TurnRate = 14.f;        // how fast she turns to her facing (exponential, per second)
	constexpr float DashTurnRate = 40.f;
	constexpr float BobHeight = 1.1f;
	constexpr float BobHz = 0.55f;
	constexpr float BreathAmount = 0.018f;
	constexpr float BreathHz = 0.3f;
	constexpr float DashLean = 16.f;
	constexpr float DashStretch = 0.12f;
	constexpr float SquashAmount = 0.16f;
	constexpr float SquashLife = 0.45f;
	constexpr float HurtLife = 0.45f;
	constexpr float HurtShake = 5.f;
	constexpr float HurtFlash = 0.9f;

	// Her outfit (OutfitParts, in this order). T7e: no board apron, no big bow, small pigtails; button eyes and shoes.
	enum EOutfit : int32 { HairCap, PigtailL, PigtailR, TieL, TieR, BowL, BowR, BowKnot, EyeL, EyeR, ShoeL, ShoeR, NumOutfit };

	// T7e: warm skin on the whole mannequin (face, neck, arms, legs); the dress is geometry on top
	const TCHAR* const SkinHex = TEXT("#FFE6D8"); // (the mannequin's material darkens and warms it: this shows as a light peach)

	// One ring of a tube round her body, in the mesh's space (Z up, she faces +Y): an ellipse at height Z
	struct FRing { float Z, Rx, Ry, Cy; };

	// The bell skirt's profile: T = 0 at the waist, 1 at the hem (a quick flare over the hips, then nearly straight)
	FRing SkirtRing(float T, float WaistZ, float HemZ)
	{
		const float F = T > 0.f ? FMath::Pow(T, 0.7f) : 0.f;
		return FRing{ WaistZ - T * (WaistZ - HemZ), 16.f + 15.f * F, 13.f + 14.f * F, 2.5f + 1.5f * F };
	}

	struct FMeshBits
	{
		TArray<FVector> Verts;
		TArray<int32> Tris;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FProcMeshTangent> Tangents;

		/** A tube (or a patch of one, angles A0..A1 with 0 at her front) through Rings, pushed Offset out; bInward faces it inward */
		void AddTube(const TArray<FRing>& Rings, float A0, float A1, int32 Segs, float Offset, bool bInward)
		{
			auto Pos = [&](int32 R, float A)
			{
				const FRing& Ring = Rings[R];
				return FVector((Ring.Rx + Offset) * FMath::Sin(A), Ring.Cy + (Ring.Ry + Offset) * FMath::Cos(A), Ring.Z);
			};
			const int32 First = Verts.Num();
			const int32 NumRings = Rings.Num();
			for (int32 R = 0; R < NumRings; ++R)
			{
				for (int32 S = 0; S <= Segs; ++S)
				{
					const float A = A0 + (A1 - A0) * S / Segs;
					const float Da = (A1 - A0) / Segs;
					const FVector P = Pos(R, A);
					const FVector AlongA = Pos(R, A + Da * 0.5f) - Pos(R, A - Da * 0.5f);
					const FVector Up = Pos(FMath::Max(R - 1, 0), A) - Pos(FMath::Min(R + 1, NumRings - 1), A);
					FVector N = FVector::CrossProduct(AlongA, Up).GetSafeNormal();
					const FVector Radial(FMath::Sin(A), FMath::Cos(A), 0.f);
					if (FVector::DotProduct(N, Radial) < 0.f)
					{
						N = -N;
					}
					if (bInward)
					{
						N = -N;
					}
					Verts.Add(P);
					Normals.Add(N);
					UVs.Add(FVector2D(A * (Rings[R].Rx + Rings[R].Ry) * 0.5f / 60.f, P.Z / 60.f));
					Tangents.Add(FProcMeshTangent(AlongA.GetSafeNormal(), false));
				}
			}
			const int32 Row = Segs + 1;
			auto Tri = [&](int32 A, int32 B, int32 C)
			{
				// The engine's front face has (B-A)x(C-A) pointing away from its normal
				const FVector Want = Normals[A] + Normals[B] + Normals[C];
				if (FVector::DotProduct(FVector::CrossProduct(Verts[B] - Verts[A], Verts[C] - Verts[A]), Want) > 0.f)
				{
					Swap(B, C);
				}
				Tris.Add(A);
				Tris.Add(B);
				Tris.Add(C);
			};
			for (int32 R = 0; R + 1 < NumRings; ++R)
			{
				for (int32 S = 0; S < Segs; ++S)
				{
					const int32 I0 = First + R * Row + S;
					const int32 I1 = I0 + 1;
					const int32 I2 = I0 + Row;
					const int32 I3 = I2 + 1;
					Tri(I0, I2, I1);
					Tri(I1, I2, I3);
				}
			}
		}

		void Commit(UProceduralMeshComponent* Mesh, int32 Section) const
		{
			Mesh->CreateMeshSection(Section, Verts, Tris, Normals, UVs, TArray<FColor>(), Tangents, false);
		}
	};

	UStaticMeshComponent* MakePart(AActor* Owner, USceneComponent* Parent, const FString& Name)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Part->SetupAttachment(Parent);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(true);
		Part->bReceivesDecals = false;
		return Part;
	}
}

AKPHeroine::AKPHeroine()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// T7f: the model hangs on Lean (facing, lean, bob, shake); the shears too, so they follow her hand
	Lean = CreateDefaultSubobject<USceneComponent>(TEXT("Lean"));
	Lean->SetupAttachment(Root);
	Model = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Model"));
	Model->SetupAttachment(Lean);
	Model->SetRelativeRotation(FRotator(0.f, -90.f, 0.f)); // the model faces +Y
	Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Model->SetGenerateOverlapEvents(false);
	Model->SetCastShadow(true);
	Model->bReceivesDecals = false;

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetRelativeRotation(FRotator(0.f, -90.f, 0.f)); // the mannequin faces +Y
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;

	Shears = CreateDefaultSubobject<USceneComponent>(TEXT("Shears"));
	Shears->SetupAttachment(Lean);
	Shears->SetRelativeLocation(KPHeroineLook::ShearsOffset);

	BladePivotA = CreateDefaultSubobject<USceneComponent>(TEXT("BladePivotA"));
	BladePivotA->SetupAttachment(Shears);
	BladePivotA->SetRelativeLocation(FVector(0.f, 0.f, 1.8f));
	BladePivotB = CreateDefaultSubobject<USceneComponent>(TEXT("BladePivotB"));
	BladePivotB->SetupAttachment(Shears);
	BladePivotB->SetRelativeLocation(FVector(0.f, 0.f, -1.8f));

	for (int32 Side = 0; Side < 2; ++Side)
	{
		USceneComponent* Pivot = Side == 0 ? BladePivotA : BladePivotB;
		ShearParts.Add(KPHeroineLook::MakePart(this, Pivot, FString::Printf(TEXT("Blade%d"), Side)));
		ShearParts.Add(KPHeroineLook::MakePart(this, Pivot, FString::Printf(TEXT("Shank%d"), Side)));
		for (int32 Segment = 0; Segment < KPHeroineLook::RingSegments; ++Segment)
		{
			ShearParts.Add(KPHeroineLook::MakePart(this, Pivot, FString::Printf(TEXT("Ring%d_%d"), Side, Segment)));
		}
	}
	ShearScrew = KPHeroineLook::MakePart(this, Shears, TEXT("Screw"));

	for (int32 Index = 0; Index < KPHeroineLook::NumOutfit; ++Index)
	{
		OutfitParts.Add(KPHeroineLook::MakePart(this, Body, FString::Printf(TEXT("Outfit%d"), Index)));
	}
	// T7e: the dress
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UProceduralMeshComponent* Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(Index == 0 ? TEXT("Skirt") : TEXT("Bodice"));
		Mesh->SetupAttachment(Body);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCastShadow(true);
		Mesh->bReceivesDecals = false;
		(Index == 0 ? Skirt : Bodice) = Mesh;
	}
	Shears->SetVisibility(false, true);
}

void AKPHeroine::BeginPlay()
{
	Super::BeginPlay();

	SetActorScale3D(FVector(KPHeroineLook::Scale));

	// T7f: the Tripo model
	if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, KPHeroineLook::ModelPath))
	{
		Model->SetStaticMesh(Mesh);
		ModelMaterial = Model->CreateDynamicMaterialInstance(0);
	}
	else
	{
		UE_LOG(LogKP, Warning, TEXT("SM_Heroine is missing; she'll be invisible apart from her shears"));
	}

	if (KPHeroineLook::bMannequin)
	{
		if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")))
		{
			Body->SetSkeletalMeshAsset(Mesh);
			SkinMaterialA = Body->GetNumMaterials() > 0 ? Body->CreateDynamicMaterialInstance(0) : nullptr;
			SkinMaterial = Body->GetNumMaterials() > 1 ? Body->CreateDynamicMaterialInstance(1) : nullptr;
			BuildOutfit();
			BuildDress();
		}
		IdleAnim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
		DashAnim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash.MM_Dash"));
	}
	else
	{
		Body->SetVisibility(false, true);
		Body->SetComponentTickEnabled(false);
	}

	BuildShears();
	if (!bLookSet)
	{
		// The seamstress (the wardrobe's first swatches)
		SetLook(KPVisuals::Hex(TEXT("#5B2A55")), KPVisuals::Hex(TEXT("#F7F3EC")), KPVisuals::Hex(TEXT("#FF78B4")), KPVisuals::Hex(TEXT("#FF3D8B")), KPVisuals::Hex(TEXT("#FF4F9A")));
	}
	ApplyLook();

	SetShearsOpen(0.f);
	SetShearsVisible(false);
	PlayIdle();
	Animate(0.f);
}

void AKPHeroine::Animate(float RealDt)
{
	using namespace KPHeroineLook;
	const UStaticMesh* Mesh = Model->GetStaticMesh();
	const float MeshHeight = Mesh ? FMath::Max(1.f, static_cast<float>(Mesh->GetBoundingBox().Max.Z)) : 98.f;
	const float ModelScale = KPVisuals::HeroHeight / (MeshHeight * Scale);
	LifeClock += RealDt;

	// Facing: she turns quickly and smoothly to the director's yaw (the gameplay facing is the actor's)
	const float TargetYaw = GetActorRotation().Yaw;
	if (!bYawSet)
	{
		VisualYaw = TargetYaw;
		bYawSet = true;
	}
	// (faster in the dash, so the shears point along the cut almost at once)
	VisualYaw += FMath::FindDeltaAngleDegrees(VisualYaw, TargetYaw) * FMath::Min(1.f, RealDt * (bDashing ? DashTurnRate : TurnRate));
	VisualYaw = FRotator::NormalizeAxis(VisualYaw);

	// Into the dash fast, out of it a little slower
	DashBlend = FMath::FInterpTo(DashBlend, bDashing ? 1.f : 0.f, RealDt, bDashing ? 18.f : 9.f);
	const float Idle = 1.f - DashBlend;

	// Idle: a gentle bob and slow breathing
	const float Bob = BobHeight * Idle * (0.5f + 0.5f * FMath::Sin(LifeClock * UE_TWO_PI * BobHz));
	const float Breath = BreathAmount * Idle * FMath::Sin(LifeClock * UE_TWO_PI * BreathHz);

	// The end of the dash: a squash that springs back
	float Squash = 0.f;
	if (SquashTime >= 0.f)
	{
		SquashTime += RealDt;
		Squash = SquashTime < SquashLife ? SquashAmount * FMath::Exp(-SquashTime * 9.f) * FMath::Cos(SquashTime * 26.f) : 0.f;
		if (SquashTime >= SquashLife)
		{
			SquashTime = -1.f;
		}
	}

	// Hurt: a quick side-to-side shake and a red flash
	float ShakeY = 0.f;
	float FlashAmount = 0.f;
	if (HurtTime >= 0.f)
	{
		HurtTime += RealDt;
		const float Left = FMath::Max(0.f, 1.f - HurtTime / HurtLife);
		ShakeY = HurtShake * Left * FMath::Sin(HurtTime * 75.f);
		FlashAmount = HurtFlash * Left * Left;
		if (HurtTime >= HurtLife)
		{
			HurtTime = -1.f;
		}
	}

	Lean->SetRelativeLocation(FVector(0.f, ShakeY, Bob));
	Lean->SetRelativeRotation(FRotator(-DashLean * DashBlend, VisualYaw - TargetYaw, 0.f));
	// The model's own axes: X her side, Y her front (the stretch goes along the dash), Z up
	const float Along = 1.f + DashStretch * DashBlend + Squash * 0.6f;
	const float Side = 1.f - DashStretch * 0.35f * DashBlend + Squash * 0.6f - Breath * 0.35f;
	const float Up = 1.f - DashStretch * 0.4f * DashBlend - Squash + Breath;
	Model->SetRelativeScale3D(FVector(Side, Along, Up) * ModelScale);

	// The shears at her right hand (in Lean's space: the model's +Y is Lean's +X)
	const FVector Hand(RightHand.Y * ModelScale, -RightHand.X * ModelScale, RightHand.Z * ModelScale * Up);
	Shears->SetRelativeLocation(Hand + ShearsFromHand);

	if (ModelMaterial)
	{
		if (!FMath::IsNearlyEqual(SelfLightSet, KPVisuals::HeroSelfLight))
		{
			SelfLightSet = KPVisuals::HeroSelfLight;
			ModelMaterial->SetScalarParameterValue(TEXT("SelfLight"), SelfLightSet);
		}
		ModelMaterial->SetScalarParameterValue(TEXT("Flash"), FlashAmount);
	}
}

void AKPHeroine::Hurt()
{
	HurtTime = 0.f;
}

void AKPHeroine::BuildShears()
{
	using namespace KPHeroineLook;
	UStaticMesh* Cube = KPVisuals::CubeMesh();
	UStaticMesh* Cylinder = KPVisuals::CylinderMesh();
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	UMaterialInstanceDynamic* Steel = KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#B9BFCC")));
	HandleMaterial = KPVisuals::Lit(this, HandleColor);
	const float SegmentLength = 2.f * RingRadius * FMath::Tan(UE_PI / RingSegments) * 1.25f;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		// Blade A swings toward +Y as the shears open, so its ring sits on the -Y side (and the other way round)
		const float Sign = Side == 0 ? 1.f : -1.f;
		UStaticMeshComponent* Blade = ShearParts[Side * PartsPerSide];
		// A flattened cone, its point forward: local Z (the cone's axis) becomes +X; width along Y, thin in Z.
		// Its back edge sits on the pivot line and the point leans in a little, so the two close like real shears.
		Blade->SetStaticMesh(Cone ? Cone : Cube);
		Blade->SetMaterial(0, Steel);
		Blade->SetRelativeLocation(FVector(BladeLength * 0.5f - 2.f, Sign * BladeWidth * 0.18f, 0.f));
		Blade->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
		Blade->SetRelativeScale3D(Cone ? FVector(BladeThick / 100.f, BladeWidth / 100.f, BladeLength / 100.f) : FVector(BladeLength / 100.f, 0.06f, 0.025f));

		// The shank from the pivot back to the ring
		UStaticMeshComponent* Shank = ShearParts[Side * PartsPerSide + 1];
		const FVector RingCenter(-RingBack, -Sign * RingSide, 0.f);
		const FVector ShankFrom(4.f, 0.f, 0.f);
		const FVector ShankTo = RingCenter + (FVector(4.f, 0.f, 0.f) - RingCenter).GetSafeNormal() * RingRadius;
		Shank->SetStaticMesh(Cube);
		Shank->SetMaterial(0, HandleMaterial);
		Shank->SetRelativeLocation((ShankFrom + ShankTo) * 0.5f);
		Shank->SetRelativeRotation((ShankTo - ShankFrom).Rotation());
		Shank->SetRelativeScale3D(FVector(FVector::Dist(ShankFrom, ShankTo) / 100.f + 0.02f, 0.055f, 0.04f));

		// A round ring lying in the blade plane, built from short cylinders
		for (int32 Segment = 0; Segment < RingSegments; ++Segment)
		{
			const float Angle = UE_TWO_PI * (Segment + 0.5f) / RingSegments;
			const FVector Tangent(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
			UStaticMeshComponent* Part = ShearParts[Side * PartsPerSide + 2 + Segment];
			Part->SetStaticMesh(Cylinder);
			Part->SetMaterial(0, HandleMaterial);
			Part->SetRelativeLocation(RingCenter + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * RingRadius);
			Part->SetRelativeRotation(FRotationMatrix::MakeFromZ(Tangent).Rotator());
			Part->SetRelativeScale3D(FVector(RingTube * 2.f / 100.f, RingTube * 2.f / 100.f, SegmentLength / 100.f));
		}
	}
	ShearScrew->SetStaticMesh(Cylinder);
	ShearScrew->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#6A6F7C"))));
	ShearScrew->SetRelativeScale3D(FVector(0.07f, 0.07f, 0.08f));
}

FVector AKPHeroine::BoneRef(const FName& BoneName) const
{
	const USkeletalMesh* Mesh = Body ? Body->GetSkeletalMeshAsset() : nullptr;
	if (!Mesh)
	{
		return FVector::ZeroVector;
	}
	const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
	const int32 Index = Ref.FindBoneIndex(BoneName);
	return Index != INDEX_NONE ? FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Index).GetLocation() : FVector::ZeroVector;
}

void AKPHeroine::PlaceOnBone(USceneComponent* Part, const FName& BoneName, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
	const USkeletalMesh* Mesh = Body ? Body->GetSkeletalMeshAsset() : nullptr;
	if (!Part || !Mesh)
	{
		return;
	}
	const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
	const int32 Index = Ref.FindBoneIndex(BoneName);
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogKP, Warning, TEXT("Her outfit: no bone %s"), *BoneName.ToString());
		return;
	}
	// Where the part should be in the mesh's space, made relative to the bone in its reference pose: it then
	// follows the bone through every animation
	const FTransform BoneSpace = FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Index);
	const FTransform Wanted(Rotation, Location, Scale);
	Part->AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, BoneName);
	Part->SetRelativeTransform(Wanted.GetRelativeTransform(BoneSpace));
}

void AKPHeroine::BuildOutfit()
{
	// The mesh's own space: Z up, she faces +Y, her left is +X. Sizes in cm (the actor's scale comes on top).
	// Chunky on purpose so it reads from the high game camera.
	using namespace KPHeroineLook;
	UStaticMesh* Sphere = KPVisuals::SphereMesh();
	HairMaterial = KPVisuals::Lit(this, HairColor);
	BowMaterial = KPVisuals::Lit(this, BowColor);
	ShoeMaterial = KPVisuals::Lit(this, DressColor);
	EyeMaterial = KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#2A1A24")));
	auto Part = [this](int32 Index) { return OutfitParts[Index].Get(); };
	auto Shape = [&](int32 Index, UStaticMesh* Mesh, UMaterialInstanceDynamic* Material)
	{
		Part(Index)->SetStaticMesh(Mesh);
		Part(Index)->SetMaterial(0, Material);
	};

	const FName HeadBone(TEXT("head"));
	const FVector Head = BoneRef(HeadBone) + FVector(0.f, 1.5f, 10.f); // the middle of her head

	// Hair (T7e): a cap over the top and back of the head (the face stays clear), two small pigtails with ties,
	// a small bow on top
	Shape(HairCap, Sphere, HairMaterial);
	PlaceOnBone(Part(HairCap), HeadBone, Head + FVector(0.f, -3.f, 2.5f), FRotator::ZeroRotator, FVector(0.235f, 0.225f, 0.225f));
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Sign = Side == 0 ? 1.f : -1.f;
		Shape(Side == 0 ? PigtailL : PigtailR, Sphere, HairMaterial);
		PlaceOnBone(Part(Side == 0 ? PigtailL : PigtailR), HeadBone, Head + FVector(Sign * 12.5f, -5.f, -4.f), FRotator(0.f, 0.f, Sign * 22.f), FVector(0.075f, 0.075f, 0.16f));
		Shape(Side == 0 ? TieL : TieR, Sphere, BowMaterial);
		PlaceOnBone(Part(Side == 0 ? TieL : TieR), HeadBone, Head + FVector(Sign * 10.5f, -4.5f, 3.f), FRotator::ZeroRotator, FVector(0.04f));
		Shape(Side == 0 ? BowL : BowR, Sphere, BowMaterial);
		PlaceOnBone(Part(Side == 0 ? BowL : BowR), HeadBone, Head + FVector(5.f + Sign * 3.8f, -2.f, 12.5f), FRotator(0.f, 0.f, Sign * -15.f), FVector(0.06f, 0.025f, 0.042f));
		// Button eyes, a little apart, just in front of the face
		Shape(Side == 0 ? EyeL : EyeR, Sphere, EyeMaterial);
		PlaceOnBone(Part(Side == 0 ? EyeL : EyeR), HeadBone, Head + FVector(Sign * 3.6f, 9.4f, 0.5f), FRotator::ZeroRotator, FVector(0.026f, 0.014f, 0.03f));
		// Round shoes in a dark shade of the dress
		const FName FootBone(Side == 0 ? TEXT("foot_l") : TEXT("foot_r"));
		const FVector Foot = BoneRef(FootBone);
		Shape(Side == 0 ? ShoeL : ShoeR, Sphere, ShoeMaterial);
		PlaceOnBone(Part(Side == 0 ? ShoeL : ShoeR), FootBone, FVector(Foot.X, Foot.Y + 6.f, 4.f), FRotator::ZeroRotator, FVector(0.1f, 0.19f, 0.08f));
	}
	Shape(BowKnot, Sphere, BowMaterial);
	PlaceOnBone(Part(BowKnot), HeadBone, Head + FVector(5.f, -2.f, 12.5f), FRotator::ZeroRotator, FVector(0.026f));
}

void AKPHeroine::BuildDress()
{
	using namespace KPHeroineLook;
	DressMaterial = KPVisuals::Plush(this, DressColor);
	BodiceMaterial = KPVisuals::Plush(this, DressColor);
	ApronMaterial = KPVisuals::Plush(this, ApronColor);

	// The mesh's own space (Z up, she faces +Y, her left is +X), sizes in cm
	const float WaistZ = BoneRef(TEXT("pelvis")).Z + 9.f;
	const float HemZ = BoneRef(TEXT("calf_l")).Z + 5.f; // just above the knee

	// The skirt: a bell from the waist to the knees, outside and inside (so the hem never shows through)
	FMeshBits SkirtBits;
	TArray<FRing> Rings;
	constexpr int32 SkirtRings = 9;
	for (int32 Index = 0; Index < SkirtRings; ++Index)
	{
		Rings.Add(SkirtRing(Index / float(SkirtRings - 1), WaistZ, HemZ));
	}
	SkirtBits.AddTube(Rings, -UE_PI, UE_PI, 32, 0.f, false);
	SkirtBits.AddTube(Rings, -UE_PI, UE_PI, 32, -0.8f, true);
	SkirtBits.Commit(Skirt, 0);

	// The apron: a thin curved panel on the front of the skirt, from the waist to most of the way down
	FMeshBits ApronBits;
	TArray<FRing> ApronRings;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		ApronRings.Add(SkirtRing(0.03f + 0.8f * Index / 7.f, WaistZ, HemZ));
	}
	ApronBits.AddTube(ApronRings, -0.9f, 0.9f, 12, 0.7f, false);
	ApronBits.Commit(Skirt, 1);

	// The waistband (the apron's ties) over the join between the skirt and the bodice
	FMeshBits BandBits;
	TArray<FRing> BandRings;
	BandRings.Add(SkirtRing(-0.05f, WaistZ + 2.f, HemZ));
	BandRings.Last().Z = WaistZ + 2.f;
	BandRings.Add(SkirtRing(0.05f, WaistZ, HemZ));
	BandBits.AddTube(BandRings, -UE_PI, UE_PI, 32, 0.9f, false);
	BandBits.Commit(Skirt, 2);
	PlaceOnBone(Skirt, TEXT("pelvis"), FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector);
	Skirt->SetMaterial(0, DressMaterial);
	Skirt->SetMaterial(1, ApronMaterial);
	Skirt->SetMaterial(2, ApronMaterial);

	// The bodice: from inside the skirt's waist up under the arms, a little shaped (waist in, chest out)
	FMeshBits BodiceBits;
	TArray<FRing> BodiceRings;
	BodiceRings.Add(FRing{ WaistZ - 5.f, 14.5f, 11.5f, 3.f });
	BodiceRings.Add(FRing{ WaistZ + 4.f, 13.8f, 10.8f, 3.5f });
	BodiceRings.Add(FRing{ WaistZ + 16.f, 15.f, 12.f, 5.f });
	BodiceRings.Add(FRing{ WaistZ + 26.f, 16.5f, 13.5f, 6.f });
	BodiceRings.Add(FRing{ WaistZ + 34.f, 16.f, 12.5f, 4.5f });
	BodiceRings.Add(FRing{ WaistZ + 40.f, 13.5f, 10.f, 2.f });
	BodiceBits.AddTube(BodiceRings, -UE_PI, UE_PI, 32, 0.f, false);
	BodiceBits.AddTube(BodiceRings, -UE_PI, UE_PI, 32, -0.6f, true);
	BodiceBits.Commit(Bodice, 0);
	PlaceOnBone(Bodice, TEXT("spine_03"), FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector);
	Bodice->SetMaterial(0, BodiceMaterial);
}

void AKPHeroine::SetLook(const FLinearColor& Dress, const FLinearColor& Apron, const FLinearColor& Hair, const FLinearColor& Bow, const FLinearColor& Handles)
{
	DressColor = Dress;
	ApronColor = Apron;
	HairColor = Hair;
	BowColor = Bow;
	HandleColor = Handles;
	bLookSet = true;
	ApplyLook();
}

void AKPHeroine::ApplyLook()
{
	// T7e: the mannequin is all skin (its first material is most of it, face included; the second is the hands
	// and inner panels); the dress is the skirt and bodice
	const FLinearColor Skin = KPVisuals::Hex(KPHeroineLook::SkinHex);
	if (SkinMaterialA)
	{
		SkinMaterialA->SetVectorParameterValue(TEXT("Paint Tint"), Skin);
	}
	if (SkinMaterial)
	{
		SkinMaterial->SetVectorParameterValue(TEXT("Paint Tint"), Skin);
	}
	KPVisuals::SetColor(DressMaterial, DressColor);
	KPVisuals::SetColor(BodiceMaterial, FMath::Lerp(DressColor, FLinearColor::White, 0.07f));
	KPVisuals::SetColor(ShoeMaterial, DressColor * 0.6f);
	KPVisuals::SetColor(ApronMaterial, ApronColor);
	KPVisuals::SetColor(HairMaterial, HairColor);
	KPVisuals::SetColor(BowMaterial, BowColor);
	KPVisuals::SetColor(HandleMaterial, HandleColor);
}

void AKPHeroine::Place(const FVector& Location, float Yaw)
{
	SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f));
}

void AKPHeroine::SetShearsVisible(bool bShow)
{
	if (bShow != bShearsShown)
	{
		bShearsShown = bShow;
		Shears->SetVisibility(bShearsShown && !bBlinkHidden, true);
	}
}

void AKPHeroine::SetShearsOpen(float Angle)
{
	const float Degrees = FMath::RadiansToDegrees(Angle);
	BladePivotA->SetRelativeRotation(FRotator(0.f, Degrees, 0.f));
	BladePivotB->SetRelativeRotation(FRotator(0.f, -Degrees, 0.f));
}

void AKPHeroine::PlayOnce(UAnimSequence* Sequence, bool bLoop)
{
	if (!Sequence || !Body->GetSkeletalMeshAsset())
	{
		return;
	}
	Body->PlayAnimation(Sequence, bLoop);
	if (UAnimInstance* Instance = Body->GetAnimInstance())
	{
		// The dash clip carries root motion; ignoring it keeps her feet where the director put her
		Instance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}
}

void AKPHeroine::PlayIdle()
{
	if (bDashing)
	{
		// T7f: the dash is over: a small squash, then she settles
		bDashing = false;
		SquashTime = 0.f;
	}
	if (!KPHeroineLook::bMannequin)
	{
		bPlayingIdle = true;
		return;
	}
	if (bPlayingIdle && Body->IsPlaying())
	{
		return;
	}
	bPlayingIdle = true;
	PlayOnce(IdleAnim, true);
}

void AKPHeroine::PlayDash()
{
	bDashing = true; // T7f
	SquashTime = -1.f;
	bPlayingIdle = false;
	PlayOnce(DashAnim, false);
}

void AKPHeroine::UpdateAnimation()
{
	if (bPlayingIdle || !KPHeroineLook::bMannequin)
	{
		return;
	}
	const UAnimSingleNodeInstance* Single = Body->GetSingleNodeInstance();
	if (!Single || !Single->IsPlaying())
	{
		PlayIdle();
	}
}

void AKPHeroine::SetBlinkHidden(bool bHide)
{
	if (bHide != bBlinkHidden)
	{
		bBlinkHidden = bHide;
		Model->SetVisibility(!bHide);
		if (KPHeroineLook::bMannequin)
		{
			Body->SetVisibility(!bHide, true);
		}
		Shears->SetVisibility(bShearsShown && !bHide, true);
	}
}

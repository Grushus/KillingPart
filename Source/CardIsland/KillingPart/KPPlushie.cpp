// Killing Part: one plushie: a procedural body with basic shapes attached.

#include "KillingPart/KPPlushie.h"
#include "KillingPart/KPMeshGen.h"
#include "KillingPart/KPTypes.h"
#include "Components/StaticMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace KPPlushieShape
{
	constexpr int32 NumStitches = 6;
	constexpr float Squash = 0.92f;
	constexpr float MaxCutOffset = 0.75f;   // in radii: a grazing cut still leaves a visible piece
	const float StitchElevations[NumStitches] = { -62.f, -40.f, -18.f, 26.f, 48.f, 70.f };

	UStaticMeshComponent* MakePart(AActor* Owner, USceneComponent* Parent, const TCHAR* Name)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(false);
		Part->bReceivesDecals = false;
		return Part;
	}

	/** Direction from the body centre for azimuth/elevation in degrees (0, 0 is the face) */
	FVector Direction(float AzimuthDeg, float ElevationDeg)
	{
		const float Az = FMath::DegreesToRadians(AzimuthDeg);
		const float El = FMath::DegreesToRadians(ElevationDeg);
		return FVector(FMath::Cos(El) * FMath::Cos(Az), FMath::Cos(El) * FMath::Sin(Az), FMath::Sin(El));
	}

	/** A point on the (slightly squashed) body surface, relative to the body centre */
	FVector Surface(float Radius, const FVector& Dir)
	{
		return FVector(Dir.X * Radius, Dir.Y * Radius, Dir.Z * Radius * Squash);
	}
}

AKPPlushie::AKPPlushie()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Root);

	BodyMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(Visual);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->SetCastShadow(true);
	BodyMesh->bReceivesDecals = false;
	BodyMesh->bUseComplexAsSimpleCollision = false; // the halves get simple convex collision at the snip
	BodyMesh->bUseAsyncCooking = false;
	EarLeft = KPPlushieShape::MakePart(this, Visual, TEXT("EarLeft"));
	EarRight = KPPlushieShape::MakePart(this, Visual, TEXT("EarRight"));
	EarLeft->SetCastShadow(true);
	EarRight->SetCastShadow(true);
	EyeLeft = KPPlushieShape::MakePart(this, Visual, TEXT("EyeLeft"));
	EyeRight = KPPlushieShape::MakePart(this, Visual, TEXT("EyeRight"));
	ShineLeft = KPPlushieShape::MakePart(this, Visual, TEXT("ShineLeft"));
	ShineRight = KPPlushieShape::MakePart(this, Visual, TEXT("ShineRight"));
	for (int32 Index = 0; Index < KPPlushieShape::NumStitches; ++Index)
	{
		const FString Name = FString::Printf(TEXT("Stitch%d"), Index);
		Stitches.Add(KPPlushieShape::MakePart(this, Visual, *Name));
	}
	CrossA = KPPlushieShape::MakePart(this, Visual, TEXT("CrossA"));
	CrossB = KPPlushieShape::MakePart(this, Visual, TEXT("CrossB"));
	Beak = KPPlushieShape::MakePart(this, Visual, TEXT("Beak"));
	Comb = KPPlushieShape::MakePart(this, Visual, TEXT("Comb"));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Feet.Add(KPPlushieShape::MakePart(this, Visual, *FString::Printf(TEXT("Foot%d"), Index)));
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Sparkles.Add(KPPlushieShape::MakePart(this, Visual, *FString::Printf(TEXT("Sparkle%d"), Index)));
	}
	CutLine = KPPlushieShape::MakePart(this, Root, TEXT("CutLine"));
	CutLine->SetVisibility(false);
}

void AKPPlushie::Init(const FKPPlushieType& Type, int32 InTypeIndex, float InRadius, const FLinearColor& InColor, float InSpeed)
{
	using namespace KPPlushieShape;

	TypeIndex = InTypeIndex;
	Radius = InRadius;
	MeshRadius = InRadius;
	bBoss = Type.Boss;
	Color = InColor;
	Speed = InSpeed;
	Wobble = FMath::FRand() * UE_TWO_PI;
	bImpostor = Type.Look.Equals(TEXT("impostor"), ESearchCase::IgnoreCase);
	BodyHeight = Radius * (bImpostor ? 0.62f : Squash); // T7d: the impostor is as flat as a pincushion

	UStaticMesh* Sphere = KPVisuals::SphereMesh();
	UStaticMesh* Cube = KPVisuals::CubeMesh();
	UStaticMesh* Cylinder = KPVisuals::CylinderMesh();

	UMaterialInstanceDynamic* Fabric = KPVisuals::Plush(this, Color);
	FabricMaterial = Fabric;
	if (Fabric)
	{
		float Value = 0.f;
		if (Fabric->GetScalarParameterValue(FHashedMaterialParameterInfo(FName(TEXT("SelfLight"))), Value))
		{
			BaseSelfLight = Value;
		}
		if (Type.SelfLight >= 0.f)
		{
			BaseSelfLight = Type.SelfLight;
			Fabric->SetScalarParameterValue(TEXT("SelfLight"), BaseSelfLight);
		}
	}
	CurrentSelfLight = BaseSelfLight;
	UMaterialInstanceDynamic* Button = KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#2B1D24")));
	UMaterialInstanceDynamic* Thread = KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#5A3346")));
	UMaterialInstanceDynamic* Shine = KPVisuals::Unlit(this, FLinearColor::White);
	ShineMaterial = Shine;
	UMaterialInstanceDynamic* Chalk = KPVisuals::Unlit(this, FLinearColor::White);

	const FVector Center(0.f, 0.f, BodyHeight);
	const float S = Radius / 50.f; // the basic sphere has radius 50
	bPatchwork = Type.Look.Equals(TEXT("patchwork"), ESearchCase::IgnoreCase);
	const bool bFirst = Type.Look.Equals(TEXT("first"), ESearchCase::IgnoreCase);
	const bool bBird = Type.Look.Equals(TEXT("hen"), ESearchCase::IgnoreCase) || Type.Look.Equals(TEXT("chick"), ESearchCase::IgnoreCase);

	// Body: generated at its real size (no component scale), with UVs for a fabric texture
	UMaterialInstanceDynamic* EarFabric[2] = { Fabric, Fabric };
	if (bPatchwork)
	{
		// Sewn from mismatched patches: neighbours never share a colour (column step 1, row step 3, of 7)
		// (stronger than the teddies' pastels, so it stands out among them)
		static const TCHAR* const PatchColors[] = { TEXT("#FF8FB8"), TEXT("#7CC4F5"), TEXT("#FFD45E"), TEXT("#8FDB88"), TEXT("#B597F0"), TEXT("#FFA87A"), TEXT("#6FDCC8") };
		constexpr int32 NumColors = UE_ARRAY_COUNT(PatchColors);
		constexpr int32 PatchesU = 4;
		constexpr int32 PatchesV = 3;
		const int32 Sections = KPMeshGen::BuildPatchBody(BodyMesh, Radius, BodyHeight, PatchesU, PatchesV);
		const int32 Start = FMath::RandRange(0, NumColors - 1);
		for (int32 Section = 0; Section < Sections; ++Section)
		{
			const int32 Row = Section / PatchesU;
			const int32 Column = Section % PatchesU;
			UMaterialInstanceDynamic* Patch = KPVisuals::Plush(this, KPVisuals::Hex(PatchColors[(Start + Column + Row * 3) % NumColors]));
			PatchMaterials.Add(Patch);
			BodyMesh->SetMaterial(Section, Patch);
		}
		EarFabric[0] = KPVisuals::Plush(this, KPVisuals::Hex(PatchColors[(Start + 5) % NumColors]));
		EarFabric[1] = KPVisuals::Plush(this, KPVisuals::Hex(PatchColors[(Start + 2) % NumColors]));
		PatchMaterials.Add(EarFabric[0]);
		PatchMaterials.Add(EarFabric[1]);
	}
	else if (bFirst)
	{
		// The first one: faded, well-loved fabric with a few patches sewn on over the years
		static const TCHAR* const OldPatches[] = { TEXT("#C9A3A0"), TEXT("#9FB2C6"), TEXT("#C2BC92") };
		constexpr int32 PatchesU = 5;
		constexpr int32 PatchesV = 3;
		const int32 Sections = KPMeshGen::BuildPatchBody(BodyMesh, Radius, BodyHeight, PatchesU, PatchesV);
		for (int32 Section = 0; Section < Sections; ++Section)
		{
			// Most of it the old fabric; three patches where it wore through
			const bool bPatch = Section == 3 || Section == 6 || Section == 12;
			UMaterialInstanceDynamic* Patch = bPatch ? KPVisuals::Plush(this, KPVisuals::Hex(OldPatches[(Section / 3) % 3])) : Fabric;
			if (bPatch)
			{
				PatchMaterials.Add(Patch);
			}
			BodyMesh->SetMaterial(Section, Patch);
		}
	}
	else
	{
		KPMeshGen::BuildBody(BodyMesh, Radius, BodyHeight);
		BodyMesh->SetMaterial(0, Fabric);
	}
	BodyMesh->SetRelativeLocation(Center);

	// Ears: round and a bit flat (teddy), or long and upright, leaning out a little (bunny); a bird's are wings
	const bool bLongEars = Type.Ears.Equals(TEXT("long"), ESearchCase::IgnoreCase);
	const float EarScale = 0.38f * S;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		UStaticMeshComponent* Ear = Side == 0 ? EarLeft : EarRight;
		const float Sign = Side == 0 ? -1.f : 1.f;
		Ear->SetStaticMesh(Sphere);
		Ear->SetMaterial(0, EarFabric[Side]);
		if (bBird)
		{
			Ear->SetRelativeLocation(Center + FVector(-0.15f * Radius, Sign * 0.9f * Radius, 0.05f * Radius));
			Ear->SetRelativeRotation(FRotator(-20.f, Sign * 10.f, 0.f));
			Ear->SetRelativeScale3D(FVector(0.45f * S, 0.14f * S, 0.3f * S));
		}
		else if (bLongEars)
		{
			// The basic sphere has radius 50, so a scale of k * S gives a radius of k * Radius
			// Leaning back (top toward -X) and out, so their length shows from the high camera
			Ear->SetRelativeLocation(Center + FVector(-0.32f * Radius, Sign * 0.36f * Radius, 1.18f * Radius));
			Ear->SetRelativeRotation(FRotator(35.f, 0.f, Sign * 16.f));
			Ear->SetRelativeScale3D(FVector(0.13f * S, 0.22f * S, 0.62f * S));
		}
		else
		{
			Ear->SetRelativeLocation(Center + FVector(-0.05f * Radius, Sign * 0.6f * Radius, 0.7f * Radius));
			Ear->SetRelativeScale3D(FVector(EarScale * 0.6f, EarScale, EarScale));
		}
	}

	// Button eyes, each with a little shine
	for (int32 Side = 0; Side < 2; ++Side)
	{
		UStaticMeshComponent* Eye = Side == 0 ? EyeLeft : EyeRight;
		UStaticMeshComponent* Dot = Side == 0 ? ShineLeft : ShineRight;
		const float Sign = Side == 0 ? -1.f : 1.f;
		const FVector Dir = Direction(Sign * 24.f, 8.f);
		const FVector Pos = Center + Surface(Radius, Dir);
		Eye->SetStaticMesh(Cylinder);
		Eye->SetMaterial(0, Button);
		Eye->SetRelativeLocation(Pos);
		Eye->SetRelativeRotation(FRotationMatrix::MakeFromZ(Dir).Rotator());
		Eye->SetRelativeScale3D(FVector(0.0034f * Radius, 0.0034f * Radius, 0.0008f * Radius));

		Dot->SetStaticMesh(Sphere);
		Dot->SetMaterial(0, Shine);
		Dot->SetRelativeLocation(Pos + Dir * (0.05f * Radius) + FVector(0.f, -0.05f * Radius, 0.06f * Radius));
		Dot->SetRelativeScale3D(FVector(0.0011f * Radius));
	}

	if (bPatchwork)
	{
		// One big button eye, and one eye that is just a big X of thread
		EyeLeft->SetRelativeScale3D(FVector(0.0048f * Radius, 0.0048f * Radius, 0.001f * Radius));
		EyeRight->SetVisibility(false);
		ShineRight->SetVisibility(false);
		const FVector Dir = Direction(24.f, 8.f);
		const FVector Pos = Center + Surface(Radius, Dir) + Dir * (0.02f * Radius);
		UMaterialInstanceDynamic* Stitch = KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#2B1D24")));
		for (int32 Arm = 0; Arm < 2; ++Arm)
		{
			UStaticMeshComponent* Cross = Arm == 0 ? CrossA : CrossB;
			Cross->SetStaticMesh(Cube);
			Cross->SetMaterial(0, Stitch);
			Cross->SetRelativeLocation(Pos);
			// Lying on the surface (X along the normal is thin), arms at +-45 degrees
			const FMatrix Facing = FRotationMatrix::MakeFromXZ(Dir, FVector::UpVector);
			const FRotator Spin = FRotator(0.f, 0.f, Arm == 0 ? 45.f : -45.f);
			Cross->SetRelativeRotation((FQuat(Facing.Rotator()) * FQuat(Spin)).Rotator());
			Cross->SetRelativeScale3D(FVector(0.0006f * Radius, 0.0052f * Radius, 0.0011f * Radius));
		}
	}

	if (bFirst)
	{
		// One button eye lost long ago and replaced with a bigger, mismatched blue one; one ear droops
		const FVector Dir = Direction(24.f, 6.f);
		EyeRight->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#5E86B3"))));
		EyeRight->SetRelativeLocation(Center + Surface(Radius, Dir));
		EyeRight->SetRelativeRotation(FRotationMatrix::MakeFromZ(Dir).Rotator());
		EyeRight->SetRelativeScale3D(FVector(0.0048f * Radius, 0.0048f * Radius, 0.001f * Radius));
		ShineRight->SetRelativeLocation(Center + Surface(Radius, Dir) + Dir * (0.06f * Radius) + FVector(0.f, -0.07f * Radius, 0.08f * Radius));
		EarRight->SetRelativeLocation(Center + FVector(-0.05f * Radius, 0.72f * Radius, 0.5f * Radius));
		EarRight->SetRelativeRotation(FRotator(0.f, 0.f, 38.f));
	}

	if (bBird)
	{
		// A beak on the face and (a hen) a red comb on top
		const FVector Dir = Direction(0.f, -2.f);
		Beak->SetStaticMesh(Sphere);
		Beak->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#FF9A3C"))));
		Beak->SetRelativeLocation(Center + Surface(Radius, Dir) + FVector(0.08f * Radius, 0.f, 0.f));
		Beak->SetRelativeScale3D(FVector(0.26f * S, 0.16f * S, 0.1f * S));
		if (Type.Look.Equals(TEXT("hen"), ESearchCase::IgnoreCase))
		{
			Comb->SetStaticMesh(Sphere);
			Comb->SetMaterial(0, KPVisuals::Plush(this, KPVisuals::Hex(TEXT("#E8344F"))));
			Comb->SetRelativeLocation(Center + FVector(0.1f * Radius, 0.f, 0.95f * BodyHeight));
			Comb->SetRelativeScale3D(FVector(0.32f * S, 0.1f * S, 0.24f * S));
		}
	}

	// A stitched seam down the middle
	for (int32 Index = 0; Index < Stitches.Num(); ++Index)
	{
		const FVector Dir = Direction(0.f, StitchElevations[Index]);
		UStaticMeshComponent* Stitch = Stitches[Index];
		Stitch->SetStaticMesh(Cube);
		Stitch->SetMaterial(0, Thread);
		Stitch->SetRelativeLocation(Center + Surface(Radius, Dir));
		Stitch->SetRelativeRotation(FRotationMatrix::MakeFromXZ(Dir, FVector::UpVector).Rotator());
		Stitch->SetRelativeScale3D(FVector(0.0004f * Radius, 0.0017f * Radius, 0.0004f * Radius));
	}

	if (bImpostor)
	{
		// T7d: a pincushion in disguise: no ears, eyes or seam; a darker band round its bottom, pins in the top,
		// and four little felt feet poking out (the tell, with the wiggle and the walk)
		UStaticMeshComponent* Hidden[] = { EarLeft.Get(), EarRight.Get(), EyeLeft.Get(), EyeRight.Get(), ShineLeft.Get(), ShineRight.Get() };
		for (UStaticMeshComponent* Part : Hidden)
		{
			Part->SetVisibility(false);
		}
		for (UStaticMeshComponent* Stitch : Stitches)
		{
			Stitch->SetVisibility(false);
		}
		const bool bGold = Type.GoldLook;
		Comb->SetStaticMesh(Cylinder);
		Comb->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(bGold ? TEXT("#B8860B") : TEXT("#8C1F33"))));
		Comb->SetRelativeLocation(FVector(0.f, 0.f, BodyHeight * 0.28f));
		Comb->SetRelativeScale3D(FVector(S * 1.0f, S * 1.0f, BodyHeight * 0.5f / 100.f));
		static const TCHAR* const HeadColors[] = { TEXT("#FF4F8F"), TEXT("#FFE14D"), TEXT("#7FD3FF"), TEXT("#FFFFFF"), TEXT("#B28CFF") };
		static const TCHAR* const GoldHeads[] = { TEXT("#FFD95E"), TEXT("#FFC93C"), TEXT("#FFF1A8") };
		const FTransform ToWorld = GetActorTransform();
		constexpr int32 NumPins = 8;
		for (int32 Pin = 0; Pin < NumPins; ++Pin)
		{
			const float Azimuth = Pin * 360.f / (NumPins - 1) + FMath::FRandRange(-15.f, 15.f);
			const float Elevation = Pin == 0 ? 90.f : FMath::FRandRange(35.f, 60.f);
			const FVector Dir = Direction(Azimuth, Elevation);
			const FVector Surf = Center + FVector(Dir.X * Radius, Dir.Y * Radius, Dir.Z * BodyHeight);
			const float Length = Radius * FMath::FRandRange(0.55f, 0.75f);
			const FLinearColor Head = KPVisuals::Hex(bGold ? GoldHeads[Pin % 3] : HeadColors[Pin % 5]);
			AddStuckPin(ToWorld.TransformPosition(Surf - Dir * (Radius * 0.1f)), ToWorld.TransformVectorNoScale(-Dir), Length, Head, 0.035f * S, 0.19f * S, !bGold);
		}
		UMaterialInstanceDynamic* Felt = KPVisuals::Plush(this, KPVisuals::Hex(bGold ? TEXT("#7A5A1E") : TEXT("#4A2A2E")));
		static const float FootAz[4] = { 38.f, -38.f, 142.f, -142.f };
		FootBase.Reset();
		for (int32 Index = 0; Index < Feet.Num(); ++Index)
		{
			const FVector Dir = Direction(FootAz[Index], 0.f);
			const FVector At = FVector(Dir.X * Radius * 0.88f, Dir.Y * Radius * 0.88f, Radius * 0.1f);
			FootBase.Add(At);
			Feet[Index]->SetStaticMesh(Sphere);
			Feet[Index]->SetMaterial(0, Felt);
			Feet[Index]->SetRelativeLocation(At);
			Feet[Index]->SetRelativeRotation(FRotator(0.f, FootAz[Index], 0.f));
			Feet[Index]->SetRelativeScale3D(FVector(0.26f * S, 0.17f * S, 0.12f * S));
			Feet[Index]->SetCastShadow(true);
		}
		for (int32 Index = 0; Index < Sparkles.Num(); ++Index)
		{
			Sparkles[Index]->SetVisibility(bGold);
			if (bGold)
			{
				Sparkles[Index]->SetStaticMesh(Sphere);
				Sparkles[Index]->SetMaterial(0, KPVisuals::Unlit(this, KPVisuals::Hex(TEXT("#FFF6C8"))));
				const float Az = UE_TWO_PI * Index / Sparkles.Num();
				Sparkles[Index]->SetRelativeLocation(Center + FVector(FMath::Cos(Az) * Radius * 0.9f, FMath::Sin(Az) * Radius * 0.9f, BodyHeight * (0.6f + 0.3f * Index)));
				Sparkles[Index]->SetRelativeScale3D(FVector(0.001f));
			}
		}
	}

	// The white cut line shown once her blade has passed through
	CutLine->SetStaticMesh(Cube);
	CutLine->SetMaterial(0, Chalk);
	CutLine->SetRelativeLocation(Center);
	CutLine->SetRelativeScale3D(FVector(0.028f * Radius, 0.04f, (2.f * BodyHeight + 14.f) / 100.f));
}

void AKPPlushie::UpdatePose(const FVector& FaceDirection, float Phase, bool bWaddle, float Lift, float Scale)
{
	if (!FaceDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, FaceDirection.Rotation().Yaw, 0.f));
	}
	const float Bob = bWaddle ? FMath::Abs(FMath::Sin(Phase)) * Radius * 0.07f : 0.f;
	const float Roll = bWaddle ? FMath::Sin(Phase) * 6.f : 0.f;
	Visual->SetRelativeLocationAndRotation(FVector(0.f, 0.f, Bob + Lift) + PoseJitter, FRotator(0.f, 0.f, Roll));
	Visual->SetRelativeScale3D(FVector(FMath::Max(Scale, 0.01f) * SizeScale));
	if (bImpostor && !bCut)
	{
		// T7d: little steps (diagonal pairs), and the golden one's twinkles flashing in turn
		for (int32 Index = 0; Index < Feet.Num() && Index < FootBase.Num(); ++Index)
		{
			const float Step = bWaddle ? FMath::Max(0.f, FMath::Sin(Phase * 2.f + ((Index == 0 || Index == 3) ? 0.f : UE_PI))) : 0.f;
			Feet[Index]->SetRelativeLocation(FootBase[Index] + FVector(0.f, 0.f, Step * Radius * 0.12f));
		}
		for (int32 Index = 0; Index < Sparkles.Num(); ++Index)
		{
			if (Sparkles[Index]->IsVisible())
			{
				const float Flash = FMath::Pow(FMath::Max(0.f, FMath::Sin(Phase * 0.7f + Index * 2.1f)), 6.f);
				const float Size = FMath::Max(0.001f, Radius / 50.f * 0.16f * Flash);
				Sparkles[Index]->SetRelativeScale3D(FVector(Size, Size, Size * 2.2f));
			}
		}
	}
}

void AKPPlushie::SetSizeScale(float InScale)
{
	SizeScale = FMath::Max(InScale, 0.05f);
	Radius = MeshRadius * SizeScale;
	Visual->SetRelativeScale3D(FVector(SizeScale));
}

void AKPPlushie::Unmark()
{
	bMarked = false;
	CutLine->SetVisibility(false);
}

void AKPPlushie::EnsureCapMaterial(UMaterialInterface* CapBase)
{
	// M_KPCap measures each pixel's distance from the body centre in units of the body's radii (mesh units,
	// so a shrunken boss still works), which is 1 at the rim for any cut, and paints bands inwards from there
	if (CapMaterial || !CapBase)
	{
		return;
	}
	CapMaterial = UMaterialInstanceDynamic::Create(CapBase, this);
	if (CapMaterial)
	{
		CapMaterial->SetVectorParameterValue(TEXT("Radii"), FLinearColor(MeshRadius, MeshRadius, BodyHeight, 0.f));
		CapMaterial->SetVectorParameterValue(TEXT("SkinColor"), Color);
		SetCapTheatrical(KPSettings::IsTheatrical());
	}
}

bool AKPPlushie::ChopChunk(const FVector& Outward, float OffsetFraction, UMaterialInterface* CapBase, const FKPTuning& Tuning, FVector& OutChunkAt)
{
	if (bCut || !BodyMesh)
	{
		return false;
	}
	EnsureCapMaterial(CapBase);
	FVector Out = Outward;
	Out.Z = 0.f;
	if (!Out.Normalize())
	{
		return false;
	}
	// How far the body still reaches on that side (an earlier chunk may have taken it already)
	const FTransform BodyToWorld = BodyMesh->GetComponentTransform();
	const FVector LocalOut = BodyToWorld.InverseTransformVectorNoScale(Out).GetSafeNormal();
	float Reach = 0.f;
	for (int32 Section = 0; Section < BodyMesh->GetNumSections(); ++Section)
	{
		if (const FProcMeshSection* Data = BodyMesh->GetProcMeshSection(Section))
		{
			for (const FProcMeshVertex& Vertex : Data->ProcVertexBuffer)
			{
				Reach = FMath::Max(Reach, static_cast<float>(FVector::DotProduct(Vertex.Position, LocalOut)));
			}
		}
	}
	const float LocalOffset = FMath::Max(0.f, FMath::Min(OffsetFraction * MeshRadius, Reach - 0.3f * MeshRadius));
	const FVector PlaneOrigin = BodyToWorld.TransformPosition(LocalOut * LocalOffset);

	// The body keeps the inner side (the plane normal points back to the centre); the chunk is the new component
	UProceduralMeshComponent* Chunk = nullptr;
	UKismetProceduralMeshLibrary::SliceProceduralMesh(BodyMesh, PlaneOrigin, -Out, true, Chunk,
		EProcMeshSliceCapOption::CreateNewSectionForCap, CapMaterial ? CapMaterial.Get() : CapBase);
	if (!Chunk)
	{
		return false;
	}
	Chunk->bReceivesDecals = false;
	Chunks.Add(Chunk);

	// Ears, eyes, stitches and pins past the plane go with the chunk
	TArray<USceneComponent*> Parts = { EarLeft.Get(), EarRight.Get(), EyeLeft.Get(), ShineLeft.Get(), EyeRight.Get(), ShineRight.Get(), CrossA.Get(), CrossB.Get(), Beak.Get(), Comb.Get() };
	for (UStaticMeshComponent* Each : Stitches)
	{
		Parts.Add(Each);
	}
	for (UStaticMeshComponent* Each : StuckPins)
	{
		Parts.Add(Each);
	}
	for (USceneComponent* Part : Parts)
	{
		if (Part && Part->GetAttachParent() == Visual && FVector::DotProduct(Part->GetComponentLocation() - PlaneOrigin, Out) > 0.0)
		{
			Part->AttachToComponent(Chunk, FAttachmentTransformRules::KeepWorldTransform);
		}
	}

	// A convex hull from its own vertices (at most about 120 points), physics, and a push outwards
	TArray<FVector> Points;
	int32 Total = 0;
	for (int32 Section = 0; Section < Chunk->GetNumSections(); ++Section)
	{
		if (const FProcMeshSection* Data = Chunk->GetProcMeshSection(Section))
		{
			Total += Data->ProcVertexBuffer.Num();
		}
	}
	const int32 Stride = FMath::Max(1, Total / 120);
	int32 Counter = 0;
	FVector Sum = FVector::ZeroVector;
	for (int32 Section = 0; Section < Chunk->GetNumSections(); ++Section)
	{
		if (const FProcMeshSection* Data = Chunk->GetProcMeshSection(Section))
		{
			for (const FProcMeshVertex& Vertex : Data->ProcVertexBuffer)
			{
				Sum += Vertex.Position;
				if (Counter++ % Stride == 0)
				{
					Points.Add(Vertex.Position);
				}
			}
		}
	}
	OutChunkAt = Chunk->GetComponentTransform().TransformPosition(Total > 0 ? Sum / Total : FVector::ZeroVector);
	if (Points.Num() >= 4)
	{
		TArray<TArray<FVector>> Hulls;
		Hulls.Add(Points);
		Chunk->bUseComplexAsSimpleCollision = false;
		Chunk->SetCollisionConvexMeshes(Hulls);
		Chunk->SetCollisionObjectType(ECC_PhysicsBody);
		Chunk->SetCollisionResponseToAllChannels(ECR_Ignore);
		Chunk->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Chunk->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Chunk->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		Chunk->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		Chunk->SetSimulatePhysics(true);
		Chunk->SetLinearDamping(Tuning.HalfDrag);
		Chunk->SetAngularDamping(1.5f);
		const float Apart = FMath::FRandRange(Tuning.HalfSpeedMin, Tuning.HalfSpeedMax) * 1.2f;
		Chunk->SetPhysicsLinearVelocity(Out * Apart + FVector(0.f, 0.f, Tuning.HalfPop * FMath::FRandRange(0.9f, 1.4f)));
		const FVector Side = FVector::CrossProduct(Out, FVector::UpVector);
		Chunk->SetPhysicsAngularVelocityInDegrees(Side * (-Tuning.HalfSpin * FMath::FRandRange(0.6f, 1.2f)) + FVector(0.f, 0.f, FMath::FRandRange(-1.f, 1.f) * Tuning.HalfSpin));
	}
	return true;
}

void AKPPlushie::SetSelfLight(float Value)
{
	if (FMath::IsNearlyEqual(Value, CurrentSelfLight, 0.002f))
	{
		return;
	}
	CurrentSelfLight = Value;
	if (FabricMaterial)
	{
		FabricMaterial->SetScalarParameterValue(TEXT("SelfLight"), Value);
	}
	for (UMaterialInstanceDynamic* Patch : PatchMaterials)
	{
		if (Patch)
		{
			Patch->SetScalarParameterValue(TEXT("SelfLight"), Value);
		}
	}
}

void AKPPlushie::AddStuckPin(const FVector& Tip, const FVector& Direction, float Length, const FLinearColor& HeadColor, float ShaftScale, float HeadScale, bool bLitHead)
{
	// Like the pincushion's pins (a steel shaft and a round head), only bigger so it reads from the camera
	const FVector Dir = Direction.GetSafeNormal();
	USceneComponent* Parent = bCut ? static_cast<USceneComponent*>(BodyMesh.Get()) : static_cast<USceneComponent*>(Visual.Get());
	UStaticMeshComponent* Shaft = NewObject<UStaticMeshComponent>(this);
	UStaticMeshComponent* Head = NewObject<UStaticMeshComponent>(this);
	UStaticMeshComponent* Parts[2] = { Shaft, Head };
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(true);
		Part->bReceivesDecals = false;
		Part->SetupAttachment(Parent);
		Part->RegisterComponent();
	}
	Shaft->SetStaticMesh(KPVisuals::CylinderMesh());
	Shaft->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#E4E8F0"))));
	Shaft->SetWorldLocationAndRotation(Tip - Dir * (Length * 0.5f), FRotationMatrix::MakeFromZ(Dir).Rotator());
	Shaft->SetWorldScale3D(FVector(ShaftScale, ShaftScale, Length / 100.f));
	Head->SetStaticMesh(KPVisuals::SphereMesh());
	Head->SetMaterial(0, bLitHead ? KPVisuals::Lit(this, HeadColor) : KPVisuals::Unlit(this, HeadColor));
	Head->SetWorldLocation(Tip - Dir * Length);
	Head->SetWorldScale3D(FVector(HeadScale));
	StuckPins.Add(Shaft);
	StuckPins.Add(Head);
}

bool AKPPlushie::RaiseChunk(int32 Index, FVector& OutAt)
{
	UProceduralMeshComponent* Chunk = Chunks.IsValidIndex(Index) ? Chunks[Index].Get() : nullptr;
	if (!Chunk || !Chunk->IsVisible())
	{
		return false;
	}
	OutAt = Chunk->Bounds.Origin;
	Chunk->SetSimulatePhysics(false);
	Chunk->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Chunk->SetVisibility(false, true); // its ear, eye or stitches go with it
	return true;
}

void AKPPlushie::SetEyeGlint(float Brightness)
{
	if (ShineMaterial && !FMath::IsNearlyEqual(Brightness, CurrentGlint, 0.01f))
	{
		CurrentGlint = Brightness;
		KPVisuals::SetColor(ShineMaterial, FLinearColor(Brightness, Brightness, Brightness * 0.9f, 1.f));
	}
}

bool AKPPlushie::GetHalfLocations(FVector& OutA, FVector& OutB) const
{
	if (!bCut || !BodyMesh || !OtherHalf)
	{
		return false;
	}
	// The halves' origins stay at the old body centre; their centres of mass are where they really are
	OutA = BodyMesh->Bounds.Origin;
	OutB = OtherHalf->Bounds.Origin;
	return true;
}

void AKPPlushie::SetMarked(float InCutYaw, float InOffset)
{
	bMarked = true;
	CutYaw = InCutYaw;
	const float MaxOffset = KPPlushieShape::MaxCutOffset * Radius;
	CutOffset = FMath::Clamp(InOffset, -MaxOffset, MaxOffset);
	Visual->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	Visual->SetRelativeScale3D(FVector(SizeScale));
	PopIn = 0.f;
	CutLine->SetWorldRotation(FRotator(0.f, CutYaw, 0.f));
	CutLine->SetWorldLocation(GetBodyCenter() + KPMath::CutNormal(CutYaw) * CutOffset);
	CutLine->SetVisibility(true);
}

FVector AKPPlushie::GetBodyCenter() const
{
	return GetActorLocation() + FVector(0.f, 0.f, BodyHeight * SizeScale);
}

bool AKPPlushie::Slice(UMaterialInterface* CapBase, const FKPTuning& Tuning)
{
	if (bCut)
	{
		return true;
	}

	// The cap (see EnsureCapMaterial; the boss already has one from its chunks)
	EnsureCapMaterial(CapBase);

	const float YawRad = FMath::DegreesToRadians(CutYaw);
	const FVector CutDir(FMath::Cos(YawRad), FMath::Sin(YawRad), 0.f);
	const FVector Normal = KPMath::CutNormal(CutYaw);
	const FVector Center = BodyMesh->GetComponentLocation();
	const FVector PlaneOrigin = Center + Normal * CutOffset;   // where her path passed closest

	// The body keeps the side the normal points to; the other side becomes a new component
	UProceduralMeshComponent* Other = nullptr;
	UKismetProceduralMeshLibrary::SliceProceduralMesh(BodyMesh, PlaneOrigin, Normal, true, Other,
		EProcMeshSliceCapOption::CreateNewSectionForCap, CapMaterial ? CapMaterial.Get() : CapBase);
	if (!Other)
	{
		return false;
	}
	OtherHalf = Other;
	OtherHalf->bReceivesDecals = false;
	bCut = true;
	CutLine->SetVisibility(false);

	// Every attached part follows the half it sits on (a shine stays with its eye)
	auto HalfFor = [&](const USceneComponent* Part) -> UProceduralMeshComponent*
	{
		return FVector::DotProduct(Part->GetComponentLocation() - PlaneOrigin, Normal) >= 0.0 ? BodyMesh.Get() : Other;
	};
	const FAttachmentTransformRules KeepWorld = FAttachmentTransformRules::KeepWorldTransform;
	// (parts already on one of the boss's chunks stay there)
	auto Move = [&](USceneComponent* Part, const USceneComponent* DecidedBy)
	{
		if (Part && Part->GetAttachParent() == Visual)
		{
			Part->AttachToComponent(HalfFor(DecidedBy), KeepWorld);
		}
	};
	Move(EarLeft, EarLeft);
	Move(EarRight, EarRight);
	Move(EyeLeft, EyeLeft);
	Move(ShineLeft, EyeLeft);
	Move(EyeRight, EyeRight);
	Move(ShineRight, EyeRight);
	for (UStaticMeshComponent* Stitch : Stitches)
	{
		Move(Stitch, Stitch);
	}
	for (int32 Index = 0; Index + 1 < StuckPins.Num(); Index += 2)
	{
		// A pin and its head go with the half the pin's middle is in
		Move(StuckPins[Index], StuckPins[Index]);
		Move(StuckPins[Index + 1], StuckPins[Index]);
	}
	UStaticMeshComponent* Extras[] = { CrossA.Get(), CrossB.Get(), Beak.Get(), Comb.Get() };
	for (UStaticMeshComponent* Part : Extras)
	{
		if (Part && Part->GetStaticMesh())
		{
			Move(Part, Part);
		}
	}
	for (UStaticMeshComponent* Foot : Feet)
	{
		if (Foot && Foot->GetStaticMesh())
		{
			Move(Foot, Foot); // T7d
		}
	}
	for (UStaticMeshComponent* Sparkle : Sparkles)
	{
		Sparkle->SetVisibility(false);
	}

	// Simple convex collision, physics, and a push apart along the normal
	const FVector LocalNormal = BodyMesh->GetComponentTransform().InverseTransformVectorNoScale(Normal);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UProceduralMeshComponent* Half = Index == 0 ? BodyMesh.Get() : Other;
		const float Sign = Index == 0 ? 1.f : -1.f;

		TArray<TArray<FVector>> Hulls;
		// (in mesh units: a shrunken boss's offset is scaled back)
		KPMeshGen::BuildPieceHull(MeshRadius, BodyHeight, LocalNormal * Sign, CutOffset / SizeScale * Sign, Hulls.AddDefaulted_GetRef());
		Half->bUseComplexAsSimpleCollision = false;
		Half->SetCollisionConvexMeshes(Hulls);
		Half->SetCollisionObjectType(ECC_PhysicsBody);
		Half->SetCollisionResponseToAllChannels(ECR_Ignore);
		Half->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Half->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Half->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		Half->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		Half->SetSimulatePhysics(true);
		Half->SetLinearDamping(Tuning.HalfDrag);

		// Two ways to land. Cut side up: a half dome rocks onto its round side by itself, helped by a tip.
		// Natural: the centre of mass moves to the middle of the cut face (the body's centre), which makes
		// the dome balance at any tilt, so the half stays where tumbling and rolling leave it: cut face
		// sideways or partly up, showing the dome, an ear or an eye and some of the inside.
		const bool bCapUp = FMath::FRand() < Tuning.HalfCapUpShare;
		float Tip = Sign * FMath::FRandRange(0.4f, 1.f);
		if (bCapUp)
		{
			Half->SetAngularDamping(1.f);
		}
		else
		{
			const FVector ComOffset = Half->GetComponentLocation() - Half->GetCenterOfMass();
			Half->SetCenterOfMass(Half->GetComponentTransform().InverseTransformVectorNoScale(ComOffset));
			Half->SetAngularDamping(3.f);
			Tip = Sign * FMath::FRandRange(-0.4f, 0.6f);
		}

		const float Apart = FMath::FRandRange(Tuning.HalfSpeedMin, Tuning.HalfSpeedMax);
		const FVector Velocity = Normal * (Sign * Apart) + CutDir * Tuning.HalfSpeedAlong
			+ FVector(0.f, 0.f, Tuning.HalfPop * FMath::FRandRange(0.6f, 1.2f));
		// Spin like the toy (around the vertical), plus the tip (around the cut direction; positive turns
		// the top away from the cut, since velocity = spin x radius and CutDir x Up = Normal)
		const FVector Spin = FVector(0.f, 0.f, FMath::FRandRange(-1.f, 1.f) * Tuning.HalfSpin)
			+ CutDir * (Tip * Tuning.HalfSpin);
		Half->SetPhysicsLinearVelocity(Velocity);
		Half->SetPhysicsAngularVelocityInDegrees(Spin);
	}
	return true;
}

void AKPPlushie::SetCapTheatrical(bool bTheatrical)
{
	if (CapMaterial)
	{
		CapMaterial->SetScalarParameterValue(TEXT("Theatrical"), bTheatrical ? 1.f : 0.f);
		if (KPVisuals::TheatricalStuffGlow >= 0.f)
		{
			CapMaterial->SetScalarParameterValue(TEXT("StuffGlow"), bTheatrical ? KPVisuals::TheatricalStuffGlow : 0.35f); // T7b: reads white, not cream
		}
	}
}

void AKPPlushie::SetPiecesScale(float Alpha)
{
	TArray<UProceduralMeshComponent*, TInlineAllocator<10>> Pieces = { BodyMesh.Get(), OtherHalf.Get() };
	for (UProceduralMeshComponent* Chunk : Chunks)
	{
		Pieces.Add(Chunk); // the boss's chunks go with its halves
	}
	for (UProceduralMeshComponent* Half : Pieces)
	{
		if (!Half)
		{
			continue;
		}
		if (!bPiecesFrozen)
		{
			Half->SetSimulatePhysics(false);
			Half->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Half->SetWorldScale3D(FVector(FMath::Max(Alpha, 0.01f) * SizeScale));
	}
	bPiecesFrozen = true;
}

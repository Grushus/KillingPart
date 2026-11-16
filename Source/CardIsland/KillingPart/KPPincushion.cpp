// Killing Part: a pincushion. See KPPincushion.h.

#include "KillingPart/KPPincushion.h"
#include "KillingPart/KPTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace KPPincushionShape
{
	constexpr int32 NumPins = 8;
	constexpr float Squash = 0.62f;     // cushion height over radius
	constexpr float PopTime = 0.25f;
	constexpr float ShrinkTime = 0.5f;
	constexpr int32 NumSparkles = 4;
	const TCHAR* const GoldHeadColors[] = { TEXT("#FFD95E"), TEXT("#FFC93C"), TEXT("#FFF1A8") };
	const TCHAR* const BlinkHeadColors[] = { TEXT("#B28CFF"), TEXT("#7A3FD1"), TEXT("#D9C2FF"), TEXT("#9B5CF0") }; // T7c
	const TCHAR* const HeadColors[] = { TEXT("#FF4F8F"), TEXT("#FFE14D"), TEXT("#7FD3FF"), TEXT("#FFFFFF"), TEXT("#B28CFF") };

	UStaticMeshComponent* MakePinPart(AActor* Owner, USceneComponent* Parent, const TCHAR* Name)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(true);
		Part->bReceivesDecals = false;
		return Part;
	}
}

AKPPincushion::AKPPincushion()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Root);
	Cushion = KPPincushionShape::MakePinPart(this, Visual, TEXT("Cushion"));
	Band = KPPincushionShape::MakePinPart(this, Visual, TEXT("Band"));
	// Each pin is a shaft and a round head
	for (int32 Index = 0; Index < KPPincushionShape::NumPins * 2; ++Index)
	{
		const FString Name = FString::Printf(TEXT("Pin%d"), Index);
		Pins.Add(KPPincushionShape::MakePinPart(this, Visual, *Name));
	}
	// T7b: twinkles for a golden one (hidden otherwise)
	for (int32 Index = 0; Index < KPPincushionShape::NumSparkles; ++Index)
	{
		const FString Name = FString::Printf(TEXT("Sparkle%d"), Index);
		UStaticMeshComponent* Sparkle = KPPincushionShape::MakePinPart(this, Visual, *Name);
		Sparkle->SetCastShadow(false);
		Sparkle->SetVisibility(false);
		Sparkles.Add(Sparkle);
	}
}

void AKPPincushion::Init(float InRadius, float InLife, bool bInGolden, bool bInBlink)
{
	using namespace KPPincushionShape;
	BlinkHeadMaterials.Reset();
	BlinkHeadBase.Reset();
	bGolden = bInGolden;
	bBlink = bInBlink && !bInGolden;
	Radius = InRadius;
	Life = InLife;
	Age = 0.f;

	UStaticMesh* Sphere = KPVisuals::SphereMesh();
	UStaticMesh* Cylinder = KPVisuals::CylinderMesh();
	const float Height = Radius * Squash;
	const float S = Radius / 50.f;

	// A plump red cushion sitting on a darker band
	Cushion->SetStaticMesh(Sphere);
	CushionMaterial = KPVisuals::Plush(this, KPVisuals::Hex(bGolden ? TEXT("#FFC93C") : bBlink ? TEXT("#8E5BD6") : TEXT("#E0344F")));
	Cushion->SetMaterial(0, CushionMaterial);
	if (CushionMaterial)
	{
		float Value = 0.f;
		if (CushionMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(FName(TEXT("SelfLight"))), Value))
		{
			BaseSelfLight = Value;
		}
		if (bGolden)
		{
			BaseSelfLight = 0.3f; // like the golden plushie
			CushionMaterial->SetScalarParameterValue(TEXT("SelfLight"), BaseSelfLight);
		}
	}
	Cushion->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.75f));
	Cushion->SetRelativeScale3D(FVector(S, S, S * Squash));
	Band->SetStaticMesh(Cylinder);
	Band->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(bGolden ? TEXT("#B8860B") : bBlink ? TEXT("#4B2A7A") : TEXT("#8C1F33"))));
	Band->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.22f));
	Band->SetRelativeScale3D(FVector(S * 0.95f, S * 0.95f, Height * 0.44f / 100.f));

	UMaterialInstanceDynamic* Steel = KPVisuals::Lit(this, KPVisuals::Hex(bGolden ? TEXT("#F2D27A") : TEXT("#D8DCE4")));
	const FVector CushionCenter(0.f, 0.f, Height * 0.75f);
	for (int32 Pin = 0; Pin < NumPins; ++Pin)
	{
		// Spread over the top: one in the middle, the rest in a ring, leaning outwards
		const float Azimuth = Pin * 360.f / (NumPins - 1) + FMath::FRandRange(-15.f, 15.f);
		const float Elevation = Pin == 0 ? 90.f : FMath::FRandRange(35.f, 60.f);
		const float Az = FMath::DegreesToRadians(Azimuth);
		const float El = FMath::DegreesToRadians(Elevation);
		const FVector Dir(FMath::Cos(El) * FMath::Cos(Az), FMath::Cos(El) * FMath::Sin(Az), FMath::Sin(El));
		const FVector Surface = CushionCenter + FVector(Dir.X * Radius, Dir.Y * Radius, Dir.Z * Height);
		const float Length = Radius * FMath::FRandRange(0.55f, 0.75f);

		UStaticMeshComponent* Shaft = Pins[Pin * 2];
		Shaft->SetStaticMesh(Cylinder);
		Shaft->SetMaterial(0, Steel);
		Shaft->SetRelativeLocation(Surface + Dir * (Length * 0.5f - Radius * 0.1f));
		Shaft->SetRelativeRotation(FRotationMatrix::MakeFromZ(Dir).Rotator());
		Shaft->SetRelativeScale3D(FVector(0.035f * S, 0.035f * S, Length / 100.f));

		UStaticMeshComponent* Head = Pins[Pin * 2 + 1];
		Head->SetStaticMesh(Sphere);
		if (bBlink)
		{
			// T7d: unlit purple heads, so in the dark they glint faintly like the plushies' button eyes (SetLit)
			const FLinearColor Base = KPVisuals::Hex(BlinkHeadColors[Pin % UE_ARRAY_COUNT(BlinkHeadColors)]) * 0.8f;
			UMaterialInstanceDynamic* HeadMaterial = KPVisuals::Unlit(this, Base);
			BlinkHeadMaterials.Add(HeadMaterial);
			BlinkHeadBase.Add(Base);
		}
		Head->SetMaterial(0, bBlink ? BlinkHeadMaterials.Last().Get() : bGolden
			? KPVisuals::Unlit(this, KPVisuals::Hex(GoldHeadColors[Pin % UE_ARRAY_COUNT(GoldHeadColors)]))
			: bBlink ? KPVisuals::Lit(this, KPVisuals::Hex(BlinkHeadColors[Pin % UE_ARRAY_COUNT(BlinkHeadColors)]))
			: KPVisuals::Lit(this, KPVisuals::Hex(HeadColors[Pin % UE_ARRAY_COUNT(HeadColors)])));
		Head->SetRelativeLocation(Surface + Dir * (Length - Radius * 0.1f));
		Head->SetRelativeScale3D(FVector(0.19f * S));
	}
	// T7b: the golden one twinkles: small unlit stars around the cushion, each flashing in turn
	SparkleSpots.Reset();
	SparklePhase.Reset();
	for (int32 Index = 0; Index < Sparkles.Num(); ++Index)
	{
		UStaticMeshComponent* Sparkle = Sparkles[Index];
		Sparkle->SetVisibility(bGolden);
		if (!bGolden)
		{
			continue;
		}
		Sparkle->SetStaticMesh(Sphere);
		Sparkle->SetMaterial(0, KPVisuals::Unlit(this, KPVisuals::Hex(TEXT("#FFF6C8"))));
		const float Az = UE_TWO_PI * Index / Sparkles.Num() + FMath::FRandRange(-0.4f, 0.4f);
		SparkleSpots.Add(CushionCenter + FVector(FMath::Cos(Az) * Radius * 0.85f, FMath::Sin(Az) * Radius * 0.85f, Height * FMath::FRandRange(0.4f, 1.3f)));
		SparklePhase.Add(Index / static_cast<float>(Sparkles.Num()));
		Sparkle->SetRelativeLocation(SparkleSpots.Last());
		Sparkle->SetRelativeScale3D(FVector(0.01f));
	}
	Visual->SetRelativeScale3D(FVector(0.01f));
}

bool AKPPincushion::Advance(float GameDt)
{
	using namespace KPPincushionShape;
	Age += GameDt;
	Life -= GameDt;
	JoltTime = FMath::Max(0.f, JoltTime - GameDt * 3.f);
	if (Life <= 0.f)
	{
		return false;
	}
	// Pop in with a little overshoot, shrink away at the end
	float Scale = 1.f;
	if (Age < PopTime)
	{
		Scale = KPMath::EaseOutBack(Age / PopTime);
	}
	if (Life < ShrinkTime)
	{
		Scale *= Life / ShrinkTime;
	}
	const float Wobble = JoltTime > 0.f ? FMath::Sin(Age * 60.f) * 12.f * JoltTime : 0.f;
	Visual->SetRelativeRotation(FRotator(Wobble, 0.f, Wobble * 0.6f));
	Visual->SetRelativeScale3D(FVector(FMath::Max(Scale, 0.01f)));
	if (bGolden)
	{
		// Each twinkle flashes briefly once every SparkleCycle game seconds, in turn
		constexpr float SparkleCycle = 1.4f;
		for (int32 Index = 0; Index < Sparkles.Num() && Index < SparklePhase.Num(); ++Index)
		{
			const float Phase = FMath::Fractional(Age / SparkleCycle + SparklePhase[Index]);
			const float Flash = Phase < 0.18f ? FMath::Sin(Phase / 0.18f * UE_PI) : 0.f;
			const float Size = Radius / 50.f * 0.16f * Flash;
			Sparkles[Index]->SetRelativeScale3D(FVector(FMath::Max(Size, 0.001f), FMath::Max(Size, 0.001f), FMath::Max(Size * 2.2f, 0.001f)));
		}
	}
	return true;
}

void AKPPincushion::Reappear(const FVector& At)
{
	// T7c: gone from here, popping in there (Advance plays the pop from Age 0)
	SetActorLocation(At);
	Age = 0.f;
	Visual->SetRelativeScale3D(FVector(0.01f));
}

void AKPPincushion::FadeOut()
{
	Life = FMath::Min(Life, KPPincushionShape::ShrinkTime);
}

void AKPPincushion::SetLit(float Amount)
{
	Amount = FMath::Clamp(Amount, 0.f, 1.f);
	if (FMath::IsNearlyEqual(Amount, CurrentLit, 0.01f))
	{
		return;
	}
	CurrentLit = Amount;
	if (bGolden || bBlink)
	{
		// T7b: in the dark only its twinkles and gold pin heads show (a glint), not the cushion. T7d: the
		// blinking one too: only its purple heads, glinting faintly
		const bool bDark = Amount < 0.03f;
		Cushion->SetVisibility(!bDark);
		Band->SetVisibility(!bDark);
		for (int32 Index = 0; Index < Pins.Num(); Index += 2)
		{
			Pins[Index]->SetVisibility(!bDark);
		}
	}
	else
	{
		SetActorHiddenInGame(Amount < 0.03f);
	}
	if (CushionMaterial)
	{
		CushionMaterial->SetScalarParameterValue(TEXT("SelfLight"), BaseSelfLight * Amount);
	}
	for (int32 Index = 0; Index < BlinkHeadMaterials.Num() && Index < BlinkHeadBase.Num(); ++Index)
	{
		KPVisuals::SetColor(BlinkHeadMaterials[Index], BlinkHeadBase[Index] * FMath::Lerp(0.3f, 1.f, Amount));
	}
}

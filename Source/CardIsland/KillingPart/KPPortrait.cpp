// Killing Part: the portrait stage for "meet a new plushie" (T6).

#include "KillingPart/KPPortrait.h"
#include "KillingPart/KPPincushion.h"
#include "KillingPart/KPPlushie.h"
#include "KillingPart/KPTypes.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"

namespace KPPortraitConst
{
	constexpr int32 TextureSize = 512;
	constexpr float Fov = 30.f;
	constexpr float Elevation = 16.f;    // degrees the camera looks down
	constexpr float FrameScale = 3.4f;   // the frame is this many subject radii across
}

AKPPortrait::AKPPortrait()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Root);
	Capture->FOVAngle = KPPortraitConst::Fov;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	// Its own fixed exposure, so the portrait doesn't depend on the room's night settings
	FPostProcessSettings& Post = Capture->PostProcessSettings;
	Post.bOverride_AutoExposureMethod = true;
	Post.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Post.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Post.AutoExposureApplyPhysicalCameraExposure = false;
	Post.bOverride_AutoExposureBias = true;
	Post.AutoExposureBias = 0.f;
	Post.bOverride_MotionBlurAmount = true;
	Post.MotionBlurAmount = 0.f;
	Post.bOverride_VignetteIntensity = true;
	Post.VignetteIntensity = 0.6f;
	Capture->ShowFlags.SetFog(false);
	Capture->ShowFlags.SetVolumetricFog(false);

	auto SetupLight = [](ULocalLightComponent* Light, const FColor& Color)
	{
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetLightColor(FLinearColor(Color));
		Light->SetCastShadows(true);
		Light->SetVisibility(false);
	};
	KeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(Root);
	SetupLight(KeyLight, FColor(255, 222, 180));
	KeyLight->SetOuterConeAngle(32.f);
	KeyLight->SetInnerConeAngle(18.f);
	RimLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RimLight"));
	RimLight->SetupAttachment(Root);
	SetupLight(RimLight, FColor(170, 200, 255));
	RimLight->SetCastShadows(false);
	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(Root);
	SetupLight(FillLight, FColor(255, 200, 230));
	FillLight->SetCastShadows(false);

	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Floor->SetupAttachment(Root);
	Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Floor->SetCastShadow(false);
	Floor->SetVisibility(false);
}

void AKPPortrait::Frame(float SubjectRadius)
{
	using namespace KPPortraitConst;
	SubjectSize = FMath::Max(SubjectRadius, 10.f);
	if (!Target)
	{
		Target = NewObject<UTextureRenderTarget2D>(this);
		Target->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
		Target->ClearColor = FLinearColor(0.02f, 0.01f, 0.03f, 1.f);
		Target->InitAutoFormat(TextureSize, TextureSize);
		Capture->TextureTarget = Target;
	}
	// The camera in front (the subject faces -X), a little above, framing FrameScale radii
	const float Distance = (FrameScale * SubjectSize * 0.5f) / FMath::Tan(FMath::DegreesToRadians(Fov * 0.5f));
	const FVector Look = StageCenter() + FVector(0.f, 0.f, SubjectSize * 0.95f);
	const float El = FMath::DegreesToRadians(Elevation);
	const FVector CameraAt = Look + FVector(-FMath::Cos(El), 0.f, FMath::Sin(El)) * Distance;
	Capture->SetWorldLocationAndRotation(CameraAt, (Look - CameraAt).Rotation());

	// Lights scaled to the subject, so a cushion and the first teddy get the same look
	// (candela per metre squared keeps the light on the subject the same at any distance)
	const float Meters = SubjectSize * 4.f / 100.f;
	const FVector KeyAt = Look + FVector(-0.75f, -0.55f, 0.95f) * SubjectSize * 4.f;
	KeyLight->SetWorldLocationAndRotation(KeyAt, (Look - KeyAt).Rotation());
	KeyLight->SetIntensity(5.f * Meters * Meters);
	KeyLight->SetAttenuationRadius(SubjectSize * 12.f);
	RimLight->SetWorldLocation(Look + FVector(0.9f, 0.6f, 0.7f) * SubjectSize * 4.f);
	RimLight->SetIntensity(6.f * Meters * Meters);
	RimLight->SetAttenuationRadius(SubjectSize * 12.f);
	FillLight->SetWorldLocation(Look + FVector(-0.8f, 0.9f, 0.1f) * SubjectSize * 4.f);
	FillLight->SetIntensity(1.2f * Meters * Meters);
	FillLight->SetAttenuationRadius(SubjectSize * 12.f);
	KeyLight->SetVisibility(true);
	RimLight->SetVisibility(true);
	FillLight->SetVisibility(true);

	// A dark felt disc under it, where the key light makes its pool
	if (!Floor->GetStaticMesh())
	{
		Floor->SetStaticMesh(KPVisuals::CylinderMesh());
		Floor->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#4A3858"))));
	}
	Floor->SetWorldLocation(StageCenter() - FVector(0.f, 0.f, 1.f));
	Floor->SetWorldScale3D(FVector(SubjectSize * 7.f / 100.f, SubjectSize * 7.f / 100.f, 0.02f));
	Floor->SetVisibility(true);

	Capture->ShowOnlyActors.Reset();
	Capture->ShowOnlyActors.Add(this);
	for (AActor* Actor : { static_cast<AActor*>(Subject.Get()), static_cast<AActor*>(Companion.Get()), static_cast<AActor*>(Pin.Get()) })
	{
		if (Actor)
		{
			Capture->ShowOnlyActors.Add(Actor);
		}
	}
	Capture->bCaptureEveryFrame = true;
	Clock = 0.f;
}

void AKPPortrait::ShowPlushie(const FKPPlushieType& Type, int32 TypeIndex, float Radius, const FLinearColor& Color,
	const FKPPlushieType* CompanionType, int32 CompanionIndex, float CompanionRadius, const FLinearColor& CompanionColor)
{
	Clear();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	float Size = Radius;
	Subject = GetWorld()->SpawnActor<AKPPlushie>(AKPPlushie::StaticClass(), StageCenter(), FRotator(0.f, 180.f, 0.f), Params);
	if (Subject)
	{
		Subject->Init(Type, TypeIndex, Radius, Color, 0.f);
	}
	bHopper = Type.Hop;
	if (CompanionType)
	{
		// Beside it, a little in front
		const FVector At = StageCenter() + FVector(-Radius * 0.6f, Radius + CompanionRadius * 1.1f, 0.f);
		Companion = GetWorld()->SpawnActor<AKPPlushie>(AKPPlushie::StaticClass(), At, FRotator(0.f, 180.f, 0.f), Params);
		if (Companion)
		{
			Companion->Init(*CompanionType, CompanionIndex, CompanionRadius, CompanionColor, 0.f);
		}
		Size = Radius + CompanionRadius * 0.9f;
	}
	Frame(Size);
	if (CompanionType)
	{
		// Centre the frame between the two
		const FVector Shift(0.f, CompanionRadius * 0.9f, 0.f);
		Capture->SetWorldLocation(Capture->GetComponentLocation() + Shift);
	}
}

void AKPPortrait::ShowPincushion(float Radius, bool bBlink, bool bGolden)
{
	Clear();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Pin = GetWorld()->SpawnActor<AKPPincushion>(AKPPincushion::StaticClass(), StageCenter(), FRotator(0.f, 30.f, 0.f), Params);
	if (Pin)
	{
		Pin->Init(Radius, 1.0e6f, bGolden, bBlink); // (T7c: the purple blinking one; T8: the golden one for the cast picture)
	}
	bHopper = false;
	Frame(Radius * 1.1f);
}

void AKPPortrait::Clear()
{
	for (AActor* Actor : { static_cast<AActor*>(Subject.Get()), static_cast<AActor*>(Companion.Get()), static_cast<AActor*>(Pin.Get()) })
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	Subject = nullptr;
	Companion = nullptr;
	Pin = nullptr;
	Capture->bCaptureEveryFrame = false;
	Capture->ShowOnlyActors.Reset();
	KeyLight->SetVisibility(false);
	RimLight->SetVisibility(false);
	FillLight->SetVisibility(false);
	Floor->SetVisibility(false);
}

void AKPPortrait::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	// A slow turn toward the camera and back, a waddle (a hop for a bunny)
	const float Turn = FMath::DegreesToRadians(28.f * FMath::Sin(Clock * 0.9f));
	const FVector Face(-FMath::Cos(Turn), FMath::Sin(Turn), 0.f);
	if (IsValid(Subject))
	{
		float Lift = 0.f;
		if (bHopper)
		{
			const float Cycle = FMath::Fmod(Clock, 1.4f);
			Lift = Cycle > 1.0f ? FMath::Sin((Cycle - 1.0f) / 0.4f * UE_PI) * Subject->Radius * 0.6f : 0.f;
		}
		Subject->UpdatePose(Face, Clock * 4.f, !bHopper, Lift, FMath::Min(1.f, Clock / 0.25f));
	}
	if (IsValid(Companion))
	{
		const FVector Other(-FMath::Cos(-Turn * 1.3f), FMath::Sin(-Turn * 1.3f), 0.f);
		Companion->UpdatePose(Other, Clock * 6.f + 1.f, true, 0.f, FMath::Min(1.f, Clock / 0.3f));
	}
	if (IsValid(Pin))
	{
		Pin->Advance(DeltaSeconds);
		Pin->SetActorRotation(FRotator(0.f, 30.f + Clock * 20.f, 0.f));
	}
}

// Killing Part: the game. See KPDirector.h for the shape of it, DESIGN.md for the rules and
// Prototype/KillingPart_Toy.html for the feel this matches.

#include "KillingPart/KPDirector.h"
#include "KillingPart/KPHeroine.h"
#include "KillingPart/KPPincushion.h"
#include "KillingPart/KPPlayerController.h"
#include "KillingPart/KPPlushie.h"
#include "KillingPart/KPPortrait.h"
#include "KillingPart/SKPHud.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Algo/Reverse.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UnrealClient.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBackgroundBlur.h"

namespace KPDirectorConst
{
	const TCHAR* const ComboNames[] = { TEXT(""), TEXT("Snip"), TEXT("Double"), TEXT("Tailored"), TEXT("Bespoke"), TEXT("Killing part") };
	const TCHAR* const Pastels[] = { TEXT("#F6E7CF"), TEXT("#DCCBF3"), TEXT("#FFD5C2"), TEXT("#C5E4FB"), TEXT("#F9CADB"), TEXT("#D4EFD0") };
	constexpr int32 NumPastels = 6;

	constexpr float ToyPx = 3.5f;        // one pixel of the toy in Unreal units
	constexpr float GridZ = 0.1f;
	constexpr float GridStep = 70.f;      // toy: a line every 20 px, a strong one every 100
	constexpr int32 GridMajorEvery = 5;
	constexpr float StainZ = 0.2f;
	constexpr float ChalkZ = 0.8f;
	constexpr float RingZ = 0.9f;
	constexpr float TrailZ = 1.2f;
	constexpr float DashLength = 32.f;   // chalk dashes (toy: 9 on, 7 off, 3 wide)
	constexpr float GapLength = 24.f;
	constexpr float LineWidth = 10.f;
	constexpr int32 RingDashes = 12;
	constexpr float TrailLife = 0.3f;
	constexpr float TrailSpacing = 18.f;
	constexpr int32 MaxParticles = 1500;
	constexpr float PieceShrinkTime = 0.35f; // the halves shrink away over the end of their life
	constexpr int32 MaxSlicesPerFrame = 3;   // a slice costs about 2 ms; bigger cuts finish over the next frames
	constexpr int32 HeroWaitFrames = 10;
	constexpr float PopInTime = 0.3f;
	constexpr float NightIntroTime = 2.2f;   // the night's name and rule stay up this long before the first wave
	constexpr int32 MaxChalkLights = 12;     // point lights made for the chalk light (Tuning.ChalkLights uses up to this many)
	constexpr float ChalkLightHeight = 110.f;
	constexpr int32 MaxCardLevel = 3;
	// The Seam ripper's pin heads: the pincushion's colours
	const TCHAR* const PinHeadColors[] = { TEXT("#FF4F8F"), TEXT("#FFE14D"), TEXT("#7FD3FF"), TEXT("#FFFFFF"), TEXT("#B28CFF") };
	constexpr int32 NumPinHeadColors = 5;
	constexpr float SeamPinLength = 130.f;   // the Seam ripper's pins, big enough to read from the camera
	constexpr float PinTrailLife = 0.3f;     // their trail (real seconds)
	constexpr float ChickHopTime = 0.55f;    // a chick's scatter hop (game seconds)
	constexpr float ChickHopHeight = 70.f;
	constexpr float ChickSafeDistance = 300.f; // chicks never spawn or land closer to her than this

	// Fallback camera if the level has no CameraActor tagged KPCamera
	const FVector CameraLocation(-1630.f, 0.f, 2614.f);
	const FRotator CameraRotation(-62.f, 0.f, 0.f);
	constexpr float CameraFov = 50.f;
	constexpr float CineFov = 55.f;          // T7d: the close camera

	const FName CameraTag(TEXT("KPCamera"));
}

AKPDirector::AKPDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	PrimaryActorTick.bTickEvenWhenPaused = true; // the pause menu runs while the world is paused

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	GridIsm = MakeIsm(TEXT("Grid"));
	GridMajorIsm = MakeIsm(TEXT("GridMajor"));
	ChalkIsm = MakeIsm(TEXT("Chalk"));
	RingIsm = MakeIsm(TEXT("Rings"));
	WarnIsm = MakeIsm(TEXT("WarnRings"));
	TrailIsm = MakeIsm(TEXT("Trail"));
	FlyPinIsm = MakeIsm(TEXT("FlyPins"));
	FlyPinHeadIsm = MakeIsm(TEXT("FlyPinHeads"));
	PinTrailIsm = MakeIsm(TEXT("PinTrail"));
	for (int32 Index = 0; Index < KPDirectorConst::NumPinHeadColors; ++Index)
	{
		const FString Name = FString::Printf(TEXT("FlyPinHeads%d"), Index);
		FlyPinHeadIsms.Add(MakeIsm(*Name));
	}
	BandIsm = MakeIsm(TEXT("CutBand"));
	SafeIsm = MakeIsm(TEXT("SafeRings"));
	BlinkRingIsm = MakeIsm(TEXT("BlinkRings"));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FString Name = FString::Printf(TEXT("BlinkLight%d"), Index);
		UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(*Name);
		Light->SetupAttachment(Root);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetCastShadows(false);
		Light->SetVisibility(false);
		BlinkLights.Add(Light);
	}
	StainIsm = MakeIsm(TEXT("Stains"));
	SplatIsm = MakeIsm(TEXT("Splats"));

	// Thursday's lamp (Lights out): hidden until that night
	Lamp = CreateDefaultSubobject<USpotLightComponent>(TEXT("Lamp"));
	Lamp->SetupAttachment(Root);
	Lamp->SetMobility(EComponentMobility::Movable);
	Lamp->SetVisibility(false);
	// Lights out: the chalk line lights the mat around it (UpdateChalkLight)
	for (int32 Index = 0; Index < KPDirectorConst::MaxChalkLights; ++Index)
	{
		const FString Name = FString::Printf(TEXT("ChalkLight%d"), Index);
		UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(*Name);
		Light->SetupAttachment(Root);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetCastShadows(false);
		Light->SetVisibility(false);
		ChalkLightComps.Add(Light);
	}
	for (int32 Kind = 0; Kind < NumKinds; ++Kind)
	{
		const FString Name = FString::Printf(TEXT("Particles%d"), Kind);
		ParticleIsms.Add(MakeIsm(*Name));
	}
}

UInstancedStaticMeshComponent* AKPDirector::MakeIsm(const TCHAR* Name)
{
	UInstancedStaticMeshComponent* Ism = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
	Ism->SetupAttachment(Root);
	Ism->SetMobility(EComponentMobility::Movable);
	Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ism->SetGenerateOverlapEvents(false);
	Ism->SetCastShadow(false);
	Ism->bReceivesDecals = false;
	return Ism;
}

void AKPDirector::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(FVector::ZeroVector);
	SetupVisuals();
	SetupAudio();
	// The save, then the title screen (T6). Test runs (-KPAutoCut, -KPNight, -KPWave) go straight into the night.
	LoadSave();
	NightIndex = Save->CurrentNight;
	ResetRun();

	// Test switches for running a build unattended: -KPGod -KPAutoCut -KPWave=5 -KPShots=6 -KPCut=theatrical
	// (-KPTitle with -KPAutoCut: the title first, screenshotted, then Continue)
#if UE_BUILD_SHIPPING
	const TCHAR* CommandLine = TEXT(""); // T8: no test switches in the release build
#else
	const TCHAR* CommandLine = FCommandLine::Get();
#endif
	int32 TestNumber = 0;
	const bool bTestRun = FParse::Param(CommandLine, TEXT("KPAutoCut")) || FParse::Value(CommandLine, TEXT("KPNight="), TestNumber) || FParse::Value(CommandLine, TEXT("KPWave="), TestNumber);
	FString TestMode;
	if (FParse::Value(CommandLine, TEXT("KPMode="), TestMode))
	{
		// -KPMode=extended (or any mode id): play that mode (T6c)
		const int32 Found = Modes.IndexOfByPredicate([&TestMode](const FKPModeInfo& Each) { return Each.Id == TestMode; });
		if (Found != INDEX_NONE)
		{
			ModeIndex = Found;
			StartNight(0);
		}
	}
	bTitleTest = FParse::Param(CommandLine, TEXT("KPTitle"));
	FString CutName;
	if (FParse::Value(CommandLine, TEXT("KPCut="), CutName))
	{
		KPSettings::SetTheatrical(CutName.StartsWith(TEXT("t"), ESearchCase::IgnoreCase));
	}
	ApplyCut(false);
	if (FParse::Param(CommandLine, TEXT("KPGod")))
	{
		bGod = true;
	}
	if (FParse::Param(CommandLine, TEXT("KPAutoCut")))
	{
		SetAutoCut(true);
		SetAutoBossOnly(FParse::Param(CommandLine, TEXT("KPBossOnly")));
	}
	int32 StartNightNumber = 0;
	if (FParse::Value(CommandLine, TEXT("KPNight="), StartNightNumber) && StartNightNumber > 0)
	{
		StartNight(StartNightNumber - 1);
	}
	FString CardList;
	if (FParse::Value(CommandLine, TEXT("KPCards="), CardList, false))
	{
		// -KPCards=extrainch,pinking,pinking (a card twice is level II)
		TArray<FString> Ids;
		CardList.ParseIntoArray(Ids, TEXT(","));
		for (const FString& Id : Ids)
		{
			TakeCard(Id.TrimStartAndEnd());
		}
	}
	int32 StartWave = 0;
	if (FParse::Value(CommandLine, TEXT("KPWave="), StartWave) && StartWave > 0)
	{
		JumpToWave(StartWave);
	}
	FParse::Value(CommandLine, TEXT("KPShots="), AutoShotsLeft);
	bBeatShots = FParse::Param(CommandLine, TEXT("KPBeatShots"));
	bCloseUpTest = FParse::Param(CommandLine, TEXT("KPCloseUp"));
	FString ShotList;
	if (FParse::Value(CommandLine, TEXT("KPShotAt="), ShotList, false))
	{
		TArray<FString> Parts;
		ShotList.ParseIntoArray(Parts, TEXT(","));
		for (const FString& Each : Parts)
		{
			ShotTimes.Add(FCString::Atof(*Each));
		}
	}
	if (!bTestRun || bTitleTest)
	{
		GoToTitle();
	}
	FString ForgetList;
	if (FParse::Value(CommandLine, TEXT("KPForget="), ForgetList, false) && Save)
	{
		// T7d test: forget introductions / toasts the test runs used up (-KPForget=impostor,newvariant:patchbunny)
		TArray<FString> Keys;
		ForgetList.ParseIntoArray(Keys, TEXT(","));
		for (const FString& Key : Keys)
		{
			Save->MetTypes.Remove(Key);
		}
		SaveProgress();
	}
	if (FParse::Param(CommandLine, TEXT("KPCastShots")))
	{
		// T8 test (with -KPAutoCut -KPShots=N): every portrait in turn, screenshotted, for the itch cast picture
		IntroQueue = { TEXT("teddy"), TEXT("bunny"), TEXT("patchwork"), TEXT("bigbear"), TEXT("hen"), TEXT("golden"), TEXT("impostor"),
			TEXT("pincushion"), TEXT("goldpin"), TEXT("blinkpin"), TEXT("first"), TEXT("patchbunny"), TEXT("bunnybear"), TEXT("goldbear"), TEXT("patchbear"), TEXT("cub") };
		if (ShowNextIntro())
		{
			Mode = EKPMode::Intro;
		}
	}
	if (FParse::Param(CommandLine, TEXT("KPWardrobe")))
	{
		GoToTitle();
		OpenWardrobe(); // T7d test
		int32 Pick = 0;
		if (FParse::Value(CommandLine, TEXT("KPWardrobePick="), Pick) && Save)
		{
			for (int32 Row = 0; Row < NumWardrobeRows; ++Row)
			{
				Save->Wardrobe.Add(FString::FromInt(Row), Pick);
			}
			bWardrobeApplied = false;
			SaveProgress();
		}
	}
}

void AKPDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveHud();
	SetLightsOut(false);
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	Super::EndPlay(EndPlayReason);
}

void AKPDirector::SetupVisuals()
{
	UStaticMesh* Cube = KPVisuals::CubeMesh();
	UStaticMesh* Cylinder = KPVisuals::CylinderMesh();
	UStaticMesh* Sphere = KPVisuals::SphereMesh();

	GridIsm->SetStaticMesh(Cube);
	GridIsm->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#B4DDCE"))));
	GridMajorIsm->SetStaticMesh(Cube);
	GridMajorIsm->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#E2F4EC"))));

	ChalkMaterial = KPVisuals::Unlit(this, FLinearColor::White);
	ChalkIsm->SetStaticMesh(Cube);
	ChalkIsm->SetMaterial(0, ChalkMaterial);

	RingIsm->SetStaticMesh(Cube);
	RingIsm->SetMaterial(0, KPVisuals::Unlit(this, FLinearColor::White));
	WarnIsm->SetStaticMesh(Cube);
	WarnIsm->SetMaterial(0, KPVisuals::Unlit(this, KPVisuals::Hex(TEXT("#FF2B4A"))));

	TrailIsm->SetStaticMesh(Cylinder);
	TrailMaterial = KPVisuals::Unlit(this, KPVisuals::Hex(TEXT("#FF78B4")));
	TrailIsm->SetMaterial(0, TrailMaterial);

	// Seam ripper pins: unlit, so they read bright in any light
	FlyPinIsm->SetStaticMesh(Cylinder);
	FlyPinIsm->SetMaterial(0, KPVisuals::Unlit(this, KPVisuals::Hex(TEXT("#F4F7FF"))));
	FlyPinHeadIsm->SetStaticMesh(Sphere);
	FlyPinHeadIsm->SetMaterial(0, KPVisuals::Unlit(this, KPVisuals::Hex(TEXT("#FF3D8B"))));
	for (int32 Index = 0; Index < FlyPinHeadIsms.Num(); ++Index)
	{
		FlyPinHeadIsms[Index]->SetStaticMesh(Sphere);
		FlyPinHeadIsms[Index]->SetMaterial(0, KPVisuals::Unlit(this, KPVisuals::Hex(KPDirectorConst::PinHeadColors[Index])));
	}
	PinTrailIsm->SetStaticMesh(Sphere);
	PinTrailIsm->SetMaterial(0, KPVisuals::Unlit(this, KPVisuals::Hex(TEXT("#FFF3A8"))));

	// Pinking shears: a pale band under the chalk showing how far the line reaches (lit, so it's dark in the dark)
	BandIsm->SetStaticMesh(Cube);
	BandIsm->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#B5DCCB"))));
	// Magnet: a gold ring on the pincushion it will cut safely (the warning ring is red)
	SafeIsm->SetStaticMesh(Cube);
	SafeIsm->SetMaterial(0, KPVisuals::Unlit(this, KPVisuals::Hex(TEXT("#FFD23F"))));
	BlinkRingIsm->SetStaticMesh(Cube);
	BlinkRingIsm->SetMaterial(0, KPVisuals::Unlit(this, KPVisuals::Hex(TEXT("#B28CFF"))));
	for (UPointLightComponent* Light : BlinkLights)
	{
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetLightColor(KPVisuals::Hex(TEXT("#B07CFF")));
		Light->SetSourceRadius(0.f);
		Light->SetAttenuationRadius(260.f);
	}

	for (UPointLightComponent* Light : ChalkLightComps)
	{
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetLightColor(FLinearColor(FColor(236, 240, 255)));
		Light->SetSourceRadius(0.f);
	}

	StainIsm->SetStaticMesh(Cylinder);
	StainIsm->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#9A0E26"))));
	SplatIsm->SetStaticMesh(Cylinder);
	SplatIsm->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#9A0E26"))));

	BurstSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/KillingPart/FX/NS_KPBurst.NS_KPBurst"));
	if (!BurstSystem)
	{
		UE_LOG(LogKP, Warning, TEXT("NS_KPBurst is missing; snips have no spray"));
	}

	// The insides of a cut plushie (M_KPCap: skin, stuffing and wet red, or white stuffing in the Theatrical cut)
	CapBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/KillingPart/Materials/M_KPCap.M_KPCap"));
	if (!CapBase)
	{
		UE_LOG(LogKP, Warning, TEXT("M_KPCap is missing; cut plushies get a plain lit cap"));
		CapBase = KPVisuals::Lit(this, KPVisuals::Hex(TEXT("#C2122F")));
	}

	const TCHAR* const ParticleColors[NumKinds] = { TEXT("#B5102B"), TEXT("#7A0A1C"), TEXT("#FFFFFF") };
	for (int32 Kind = 0; Kind < NumKinds; ++Kind)
	{
		ParticleIsms[Kind]->SetStaticMesh(Sphere);
		ParticleIsms[Kind]->SetMaterial(0, KPVisuals::Lit(this, KPVisuals::Hex(ParticleColors[Kind])));
	}
}

void AKPDirector::BuildGrid()
{
	// The cutting mat's printed grid, centred on the origin, plus the two 45 degree guides
	using namespace KPDirectorConst;
	const float Hx = Tuning.ArenaHalfX;
	const float Hy = Tuning.ArenaHalfY;
	TArray<FTransform> Minor;
	TArray<FTransform> Major;
	auto Line = [](TArray<FTransform>& Out, const FVector2D& A, const FVector2D& B, float Width)
	{
		const FVector2D Mid = (A + B) * 0.5;
		const float Length = FVector2D::Distance(A, B);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X));
		Out.Add(FTransform(FRotator(0.f, Yaw, 0.f), FVector(Mid.X, Mid.Y, GridZ), FVector(Length / 100.f, Width / 100.f, 0.002f)));
	};
	const int32 StepsY = FMath::FloorToInt(Hy / GridStep);
	for (int32 K = -StepsY; K <= StepsY; ++K)
	{
		const bool bMajor = K % GridMajorEvery == 0;
		Line(bMajor ? Major : Minor, FVector2D(-Hx, K * GridStep), FVector2D(Hx, K * GridStep), bMajor ? 5.f : 3.f);
	}
	const int32 StepsX = FMath::FloorToInt(Hx / GridStep);
	for (int32 K = -StepsX; K <= StepsX; ++K)
	{
		const bool bMajor = K % GridMajorEvery == 0;
		Line(bMajor ? Major : Minor, FVector2D(K * GridStep, -Hy), FVector2D(K * GridStep, Hy), bMajor ? 5.f : 3.f);
	}
	// Border
	Line(Major, FVector2D(-Hx, -Hy), FVector2D(-Hx, Hy), 6.f);
	Line(Major, FVector2D(Hx, -Hy), FVector2D(Hx, Hy), 6.f);
	Line(Major, FVector2D(-Hx, -Hy), FVector2D(Hx, -Hy), 6.f);
	Line(Major, FVector2D(-Hx, Hy), FVector2D(Hx, Hy), 6.f);
	// Diagonals from the far corners, like the toy
	const float Span = 2.f * Hx;
	Line(Minor, FVector2D(Hx, -Hy), FVector2D(-Hx, -Hy + Span), 3.f);
	Line(Minor, FVector2D(Hx, Hy), FVector2D(-Hx, Hy - Span), 3.f);
	SetInstances(GridIsm, Minor);
	SetInstances(GridMajorIsm, Major);
}

// ---------------------------------------------------------------------------
// Run

void AKPDirector::ResetRun()
{
	StartNight(NightIndex);
}

void AKPDirector::StartNight(int32 InNightIndex)
{
	if (bPaused)
	{
		SetPaused(false);
	}
	bAttract = false; // (GoToTitle turns it back on after this)
	IntroQueue.Reset();
	if (Portrait)
	{
		Portrait->Clear();
	}
	LoadData();
	NightIndex = ((InNightIndex % Nights.Num()) + Nights.Num()) % Nights.Num();
	Bosses.Reset();
	if (IsEndless())
	{
		// T6c: an Extended cut run is one long night; its rounds are made as they come (EnsureEndlessRound)
		NightIndex = 0;
		EndlessNight = FKPNight();
		EndlessNight.Name = CurrentMode().Name;
		EndlessNight.Rule = CurrentMode().Dark ? TEXT("lightsout") : TEXT("");
		EndlessNight.RuleName = FString();
		EndlessNight.RuleText = FString();
		EndlessNight.GoldenChance = Endless.GoldenChance;
		EndlessNight.PinsInPath = Endless.PinsInPath;
		EndlessNight.Mix.Add(TEXT("teddy"), 1.f);
		EnsureEndlessRound(1);
	}
	Held.Reset();
	Offer.Reset();
	HoveredCard = -1;
	ApplyCards();
	// The night's best combo starts from nothing; the HUD's best is this night's best ever
	BestCombo = 0;
	Best = Save ? Save->GetNightBest(NightIndex) : Best;
	if (IsEndless())
	{
		const FKPModeSave* Run = GetModeSave(ModeIndex);
		Best = Run ? Run->Record : 0; // (Extended cut: the best score)
	}
	if (Save)
	{
		Save->FurthestNight = FMath::Max(Save->FurthestNight, NightIndex);
	}
	UiClock = 0.f;
	BuildGrid();

	ClearPlushies();
	Marked.Reset();
	Path.Reset();
	PathLength = 0.f;
	Particles.Reset();
	Trail.Reset();
	Texts.Reset();
	StainIsm->ClearInstances();
	StainCursor = 0;
	SplatIsm->ClearInstances();
	SplatCursor = 0;

	HeroPos = FVector::ZeroVector;
	HeroYaw = 180.f; // facing the camera
	// T6c: the mode's hearts (Final cut: 1); the modes that carry them start with what the last night left
	const FKPModeInfo& ModeInfo = CurrentMode();
	Hearts = ModeInfo.Hearts > 0 ? ModeInfo.Hearts : Tuning.Hearts;
	if (ModeInfo.CarryHearts && NightIndex > 0)
	{
		const FKPModeSave* Week = GetModeSave(ModeIndex);
		if (Week && Week->Active && Week->Night == NightIndex)
		{
			Hearts = FMath::Clamp(Week->Hearts, 1, Tuning.MaxHearts);
		}
	}
	Score = 0;
	Wave = 0;
	Queue = 0;
	bBreakDone = true;
	BreakTimer = 0.f;
	SpawnTimer = 0.f;
	// T6c: a 3-2-1 over the mat (the banner shows meanwhile); T6d: only after Continue (a loaded save)
	const bool bWantCountdown = bCountdownNext && !bAttract;
	bCountdownNext = false;
	bCountdownAfterIntro = false;
	Countdown = bWantCountdown ? Tuning.Countdown : 0.f;
	PlayTime = 0.0;
	WaveTimer = Tuning.FirstWaveDelay;
	Mode = EKPMode::Play;
	Ts = 1.f;
	TsTarget = 1.f;
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	Cooldown = 0.f;
	Invulnerable = 0.f;
	SnipTimer = 0.f;
	Shake = 0.f;
	CineMode = 0;
	HitStopTimer = 0.f;
	GoldenPinsCut = 0;
	GoldenPinSpots.Reset();
	WaveSpawned = 0;
	WaveGolden = 0;
	WaveGoldenRolled = 0;
	Flash = 0.f;
	PreviewCount = 0;
	bReturning = false;
	FlyingPins.Reset();
	SpawnList.Reset();
	PathPinsLeft = 0;
	LitLine.Reset();
	ChalkLightLevel = 0.f;
	// The tutorial: Monday's first wave, until it has been shown once
	TutorialStep = (NightIndex == 0 && Save && !Save->TutorialCut && !bAttract && !IsEndless()) ? 1 : 0;
	TutorialTimer = 0.f;
	TutorialFade = 0.f;
	BossPlushie.Reset();
	BossCutsTaken = 0;
	BossHitsLeft = 0;
	ImpostorRollWave = -1;
	bImpostorRoll = false;
	ImpostorTimer = -1.f;
	PendingSpawns = 0;
	PendingChicks = 0;
	FinaleTimer = 0.f;
	bBossDown = false;

	if (AKPHeroine* H = Hero.Get())
	{
		H->SetBlinkHidden(false);
		H->PlayIdle();
	}

	// The night's rule: only Lights out needs code; the others are in its waves (types, pins, chalk)
	const FKPNight& Night = CurrentNight();
	SetLightsOut(Night.Rule.Equals(TEXT("lightsout"), ESearchCase::IgnoreCase));
	TutorialShown.Reset();
	BossChunks.Reset();
	bBossEnraged = false;
	BossTellTimer = 0.f;
	// T6b: when the first wave brings someone new, meet them first (straight away), then the banner without
	// its description line (the introduction says it better)
	bNightBannerPending = false;
	if (!bAttract && Save)
	{
		TArray<FString> Newcomers;
		EnsureImpostorRoll(1);
		FindNewcomers(1, Newcomers);
		bNightBannerPending = Newcomers.Num() > 0;
	}
	if (bNightBannerPending)
	{
		WaveTimer = 0.4f;
		Countdown = 0.f; // (after the introduction)
		bCountdownAfterIntro = bWantCountdown;
	}
	// (T7d: the curse modes pick their curses after each wave, in OpenPick)
	bCursePickAtStart = false;
	// T6c: Extreme: one random card at the start of each night
	if (!bAttract && ModeInfo.NightCard)
	{
		TArray<int32> Pool;
		for (int32 Index = 0; Index < Cards.Num(); ++Index)
		{
			if (Cards[Index].Pool && !(Cards[Index].Id == TEXT("thimble") && Tuning.Hearts >= Tuning.MaxHearts))
			{
				Pool.Add(Index);
			}
		}
		if (Pool.Num() > 0)
		{
			TakeCard(Cards[Pool[FMath::RandRange(0, Pool.Num() - 1)]].Id);
		}
	}
	else
	{
		ShowNightBanner(true);
	}
	UE_LOG(LogKP, Log, TEXT("Night %d: %s (%d waves)"), NightIndex + 1, *Night.Name, Night.Waves.Num());
}

void AKPDirector::ShowNightBanner(bool bWithText)
{
	const FKPNight& Night = CurrentNight();
	AddScreenText(FVector2D(0.5, 0.14), Night.Name, KPDirectorConst::NightIntroTime + 0.6f, 32.f, KPVisuals::Hex(TEXT("#FFE14D")));
	if (!Night.RuleName.IsEmpty())
	{
		AddScreenText(FVector2D(0.5, 0.21), Night.RuleName, KPDirectorConst::NightIntroTime + 0.6f, 20.f, FLinearColor::White);
	}
	if (bWithText && !Night.RuleText.IsEmpty())
	{
		AddScreenText(FVector2D(0.5, 0.26), Night.RuleText, KPDirectorConst::NightIntroTime + 0.6f, 13.f, FLinearColor::White);
	}
}

const FKPNight& AKPDirector::CurrentNight() const
{
	static const FKPNight Fallback;
	if (IsEndless())
	{
		return EndlessNight;
	}
	return Nights.IsValidIndex(NightIndex) ? Nights[NightIndex] : Fallback;
}

const FKPWave& AKPDirector::CurrentWave() const
{
	static const FKPWave Fallback;
	const FKPNight& Night = CurrentNight();
	return Night.Waves.Num() > 0 ? Night.Waves[FMath::Clamp(Wave - 1, 0, Night.Waves.Num() - 1)] : Fallback;
}

FString AKPDirector::NextNightName() const
{
	return Nights.Num() > 0 ? Nights[(NightIndex + 1) % Nights.Num()].Name : FString();
}

void AKPDirector::ApplyCards()
{
	// The cards' numbers are DESIGN.md's; the words on them are in KillingPart.json ("cards")
	Tuning = BaseTuning;
	Tuning.Chalk *= CurrentNight().ChalkMul;
	SpeedMul = 1.f;
	ScoreMul = 1.f;
	ExtraPerWave = 0;
	ExtraPins = 0;
	bDoubleStitch = false;
	bSeamRipper = false;
	bExtraInch = false;
	ExtraInchLevel = 0;
	SeamLevel = 0;
	MagnetLevel = 0;
	WaveCountMul = 1.f;
	// A card held twice or three times is level II or III: each level adds its boon and its loose thread
	// again (boons add up: +25% chalk at level II is +50%, not x1.5625)
	TMap<FString, int32> Levels;
	for (const int32 Index : Held)
	{
		Levels.FindOrAdd(Cards[Index].Id) += 1;
	}
	const float Chalk = Tuning.Chalk;
	float ChalkScale = 1.f;
	for (const TPair<FString, int32>& Entry : Levels)
	{
		const FString& Id = Entry.Key;
		const int32 Level = Entry.Value;
		// Every number is the card's "values" in KillingPart.json (percentages as whole numbers)
		auto V = [&](const TCHAR* Key, float Default) { return CardValue(Id, Key, Default); };
		const FKPCard* CardInfo = Cards.FindByPredicate([&Id](const FKPCard& Each) { return Each.Id == Id; });
		if (CardInfo && CardInfo->Curse)
		{
			// T7c: a curse: only a loose thread, from its values
			for (int32 Each = 0; Each < Level; ++Each)
			{
				SpeedMul *= 1.f + V(TEXT("speed"), 0.f) / 100.f;
				ChalkScale -= V(TEXT("chalk"), 0.f) / 100.f;
				WaveCountMul += V(TEXT("plushies"), 0.f) / 100.f;
				ExtraPins += FMath::RoundToInt(V(TEXT("pincushions"), 0.f));
			}
			continue;
		}
		for (int32 Each = 0; Each < Level; ++Each)
		{
			if (Id == TEXT("longchalk"))
			{
				ChalkScale += V(TEXT("chalk"), 25.f) / 100.f;
				SpeedMul *= 1.f + V(TEXT("speed"), 10.f) / 100.f;
			}
			else if (Id == TEXT("pinking"))
			{
				ChalkScale -= V(TEXT("chalk"), 10.f) / 100.f;
			}
			else if (Id == TEXT("thimble"))
			{
				Tuning.Hearts = FMath::Min(Tuning.Hearts + FMath::RoundToInt(V(TEXT("hearts"), 1.f)), Tuning.MaxHearts);
				WaveCountMul += V(TEXT("plushies"), 30.f) / 100.f;
			}
			else if (Id == TEXT("extrainch"))
			{
				SpeedMul *= 1.f + V(TEXT("speed"), 10.f) / 100.f;
			}
			else if (Id == TEXT("tapemeasure"))
			{
				ScoreMul += V(TEXT("score"), 50.f) / 100.f;
				SpeedMul *= 1.f + V(TEXT("speed"), 5.f) / 100.f;
			}
			else if (Id == TEXT("seamripper"))
			{
				ExtraPins += FMath::RoundToInt(V(TEXT("pincushions"), 1.f));
			}
			else if (Id == TEXT("magnet"))
			{
				ChalkScale -= V(TEXT("chalk"), 15.f) / 100.f;
			}
			else if (Id == TEXT("doublestitch"))
			{
				// Parked (T4c): out of the card pool, still works with kp.Card doublestitch
			}
			else
			{
				UE_LOG(LogKP, Warning, TEXT("Card %s has no effect in code"), *Id);
				break;
			}
		}
		if (Id == TEXT("pinking"))
		{
			Tuning.BladePad *= 1.f + (V(TEXT("width"), 2.f) - 1.f) * Level;   // width 2: 2x, 3x, 4x as wide
		}
		bExtraInch = bExtraInch || Id == TEXT("extrainch");
		ExtraInchLevel = Id == TEXT("extrainch") ? Level : ExtraInchLevel;
		SeamLevel = Id == TEXT("seamripper") ? Level : SeamLevel;
		MagnetLevel = Id == TEXT("magnet") ? FMath::RoundToInt(V(TEXT("pincushions"), 1.f) * Level) : MagnetLevel;
		bDoubleStitch = bDoubleStitch || Id == TEXT("doublestitch");
	}
	bSeamRipper = SeamLevel > 0;
	Tuning.Chalk = Chalk * FMath::Max(ChalkScale, 0.3f);
}

float AKPDirector::CardValue(const FString& Id, const TCHAR* Key, float Default) const
{
	const FKPCard* Card = Cards.FindByPredicate([&Id](const FKPCard& Each) { return Each.Id == Id; });
	const float* Value = Card ? Card->Values.Find(Key) : nullptr;
	return Value ? *Value : Default;
}

void AKPDirector::GetActiveEffects(TArray<FString>& Boons, TArray<FString>& Threads) const
{
	// T7e: the same sums as ApplyCards (each level adds its boon and loose thread again; speed multiplies)
	TMap<FString, int32> Levels;
	for (const int32 Index : Held)
	{
		if (Cards.IsValidIndex(Index))
		{
			Levels.FindOrAdd(Cards[Index].Id) += 1;
		}
	}
	float BoonChalk = 0.f, ScoreUp = 0.f, Speed = 1.f, LessChalk = 0.f, Crowd = 0.f, Width = 1.f, ConeAngle = 0.f;
	int32 HeartsUp = 0, MorePins = 0, Magnet = 0, Seam = 0;
	bool bDouble = false;
	for (const TPair<FString, int32>& Entry : Levels)
	{
		const FString& Id = Entry.Key;
		const int32 Level = Entry.Value;
		auto V = [&](const TCHAR* Key, float Default) { return CardValue(Id, Key, Default); };
		const FKPCard* CardInfo = Cards.FindByPredicate([&Id](const FKPCard& Each) { return Each.Id == Id; });
		if (CardInfo && CardInfo->Curse)
		{
			for (int32 Each = 0; Each < Level; ++Each)
			{
				Speed *= 1.f + V(TEXT("speed"), 0.f) / 100.f;
				LessChalk += V(TEXT("chalk"), 0.f);
				Crowd += V(TEXT("plushies"), 0.f);
				MorePins += FMath::RoundToInt(V(TEXT("pincushions"), 0.f));
			}
			continue;
		}
		for (int32 Each = 0; Each < Level; ++Each)
		{
			if (Id == TEXT("longchalk")) { BoonChalk += V(TEXT("chalk"), 25.f); Speed *= 1.f + V(TEXT("speed"), 10.f) / 100.f; }
			else if (Id == TEXT("pinking")) { LessChalk += V(TEXT("chalk"), 10.f); }
			else if (Id == TEXT("thimble")) { HeartsUp += FMath::RoundToInt(V(TEXT("hearts"), 1.f)); Crowd += V(TEXT("plushies"), 30.f); }
			else if (Id == TEXT("extrainch")) { Speed *= 1.f + V(TEXT("speed"), 10.f) / 100.f; }
			else if (Id == TEXT("tapemeasure")) { ScoreUp += V(TEXT("score"), 50.f); Speed *= 1.f + V(TEXT("speed"), 5.f) / 100.f; }
			else if (Id == TEXT("seamripper")) { MorePins += FMath::RoundToInt(V(TEXT("pincushions"), 1.f)); }
			else if (Id == TEXT("magnet")) { LessChalk += V(TEXT("chalk"), 15.f); }
		}
		if (Id == TEXT("pinking"))
		{
			Width = 1.f + (V(TEXT("width"), 2.f) - 1.f) * Level;
		}
		else if (Id == TEXT("extrainch"))
		{
			ConeAngle = V(*FString::Printf(TEXT("angle%d"), FMath::Clamp(Level, 1, 3)), 30.f);
		}
		else if (Id == TEXT("seamripper"))
		{
			Seam = Level;
		}
		else if (Id == TEXT("magnet"))
		{
			Magnet = FMath::RoundToInt(V(TEXT("pincushions"), 1.f) * Level);
		}
		bDouble = bDouble || Id == TEXT("doublestitch");
	}
	auto Fill = [this](const TCHAR* Key, const TCHAR* Fallback, const FString& N)
	{
		return Text(Key, Fallback).Replace(TEXT("{n}"), *N);
	};
	auto Num = [](float Value)
	{
		return FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value), 0.05f) ? FString::FromInt(FMath::RoundToInt(Value)) : FString::Printf(TEXT("%.1f"), Value);
	};
	if (BoonChalk > 0.f) { Boons.Add(Fill(TEXT("fxChalk"), TEXT("+{n}% chalk"), Num(BoonChalk))); }
	if (Width > 1.f) { Boons.Add(Fill(TEXT("fxWidth"), TEXT("Line cuts {n}x wider"), Num(Width))); }
	if (HeartsUp > 0) { Boons.Add(HeartsUp == 1 ? Fill(TEXT("fxHeart"), TEXT("+{n} heart"), Num(HeartsUp)) : Fill(TEXT("fxHearts"), TEXT("+{n} hearts"), Num(HeartsUp))); }
	if (ScoreUp > 0.f) { Boons.Add(Fill(TEXT("fxScore"), TEXT("+{n}% score"), Num(ScoreUp))); }
	if (ConeAngle > 0.f) { Boons.Add(Fill(TEXT("fxCone"), TEXT("Line end cuts a {n} degree cone"), Num(ConeAngle))); }
	if (Seam > 0) { Boons.Add(Fill(TEXT("fxSeam"), TEXT("Cut plushies throw pins ({n} reach)"), Num(CardValue(TEXT("seamripper"), TEXT("reach"), 150.f) + CardValue(TEXT("seamripper"), TEXT("reachPerLevel"), 75.f) * (Seam - 1)))); }
	if (Magnet > 0) { Boons.Add(Magnet == 1 ? Fill(TEXT("fxMagnet"), TEXT("First pincushion cut each wave is harmless"), Num(Magnet)) : Fill(TEXT("fxMagnets"), TEXT("First {n} pincushions cut each wave are harmless"), Num(Magnet))); }
	if (bDouble) { Boons.Add(Text(TEXT("fxDouble"), TEXT("Cuts the line twice"))); }
	if (Speed > 1.001f) { Threads.Add(Fill(TEXT("fxSpeed"), TEXT("Plushies +{n}% speed"), Num(FMath::RoundToFloat((Speed - 1.f) * 100.f)))); }
	if (LessChalk > 0.f) { Threads.Add(Fill(TEXT("fxLessChalk"), TEXT("-{n}% chalk"), Num(LessChalk))); }
	if (Crowd > 0.f) { Threads.Add(Fill(TEXT("fxCrowd"), TEXT("+{n}% plushies per wave"), Num(Crowd))); }
	if (MorePins > 0) { Threads.Add(MorePins == 1 ? Fill(TEXT("fxPin"), TEXT("+{n} pincushion per wave"), Num(MorePins)) : Fill(TEXT("fxPins"), TEXT("+{n} pincushions per wave"), Num(MorePins))); }
}

FString AKPDirector::CardText(int32 CardIndex, bool bBoon) const
{
	if (!Cards.IsValidIndex(CardIndex))
	{
		return FString();
	}
	const FKPCard& Card = Cards[CardIndex];
	FString Text = bBoon ? Card.Boon : Card.Thread;
	// "{chalk}" becomes the card's number; {maxHearts} the heart cap
	auto Number = [](float Value)
	{
		return FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value)) ? FString::Printf(TEXT("%d"), FMath::RoundToInt(Value)) : FString::Printf(TEXT("%.1f"), Value);
	};
	for (const TPair<FString, float>& Entry : Card.Values)
	{
		Text.ReplaceInline(*(TEXT("{") + Entry.Key + TEXT("}")), *Number(Entry.Value));
	}
	Text.ReplaceInline(TEXT("{maxHearts}"), *Number(static_cast<float>(BaseTuning.MaxHearts)));
	return Text;
}

bool AKPDirector::TakeCard(const FString& IdOrName)
{
	auto Squash = [](const FString& Text) { return Text.Replace(TEXT(" "), TEXT("")).Replace(TEXT("'"), TEXT("")).ToLower(); };
	const FString Wanted = Squash(IdOrName);
	const int32 Index = Cards.IndexOfByPredicate([&](const FKPCard& Card) { return Squash(Card.Id) == Wanted || Squash(Card.Name) == Wanted; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	if (GetCardLevel(Index) < KPDirectorConst::MaxCardLevel)
	{
		const int32 OldMaxHearts = Tuning.Hearts;
		const int32 OldMagnet = MagnetLevel;
		Held.Add(Index);
		ApplyCards();
		if (Tuning.Hearts > OldMaxHearts)
		{
			Hearts = FMath::Min(Hearts + (Tuning.Hearts - OldMaxHearts), Tuning.Hearts); // Thimble: the new heart comes full
		}
		MagnetLeft += MagnetLevel - OldMagnet; // a new Magnet works from this wave on
		const FString Name = KPCardText::Title(Cards[Index].Name, GetCardLevel(Index));
		if (Cards[Index].Curse)
		{
			// T7c: a curse: its name in rose, and what it does under it
			AddScreenText(FVector2D(0.5, 0.3), Name, 2.2f, 22.f, KPVisuals::Hex(TEXT("#FF6E82")));
			AddScreenText(FVector2D(0.5, 0.35), CardText(Index, false), 2.2f, 14.f, KPVisuals::Hex(TEXT("#FF6E82")));
		}
		else
		{
			AddScreenText(FVector2D(0.5, 0.3), Name, 1.4f, 22.f, KPVisuals::Hex(TEXT("#FFE14D")));
		}
		UE_LOG(LogKP, Log, TEXT("Card: %s"), *Name);
	}
	if (Mode == EKPMode::Pick)
	{
		Mode = EKPMode::Play;
		Cooldown = 0.25f;
		if (bCursePickAtStart)
		{
			// T7c: the night's curse is picked: now the night's banner
			bCursePickAtStart = false;
			if (!bNightBannerPending)
			{
				ShowNightBanner(true);
			}
		}
		if (TutorialStep == 4 && Save)
		{
			Save->TutorialPick = true;
			SaveProgress();
			TutorialStep = 0;
		}
	}
	Offer.Reset();
	HoveredCard = -1;
	return true;
}

void AKPDirector::OpenPick()
{
	// T7d: the curse modes (Demon, Impossible): after each wave a curse instead of a card: pick 1 of Curses, or
	// (Curses 1) one at random, taken at once. A curse already held comes again as II, then III; all reset every night.
	if (CurrentMode().Curses > 0)
	{
		TArray<int32> CursePool;
		for (int32 Index = 0; Index < Cards.Num(); ++Index)
		{
			if (Cards[Index].Curse && GetCardLevel(Index) < KPDirectorConst::MaxCardLevel)
			{
				CursePool.Add(Index);
			}
		}
		for (int32 Index = CursePool.Num() - 1; Index > 0; --Index)
		{
			CursePool.Swap(Index, FMath::RandRange(0, Index));
		}
		if (CursePool.Num() == 0)
		{
			return;
		}
		if (CurrentMode().Curses == 1)
		{
			TakeCard(Cards[CursePool[0]].Id);
			return;
		}
		Offer.Reset();
		for (int32 Index = 0; Index < FMath::Min(CurrentMode().Curses, CursePool.Num()); ++Index)
		{
			Offer.Add(CursePool[Index]);
		}
		Mode = EKPMode::Pick;
		TsTarget = 1.f;
		HoveredCard = -1;
		return;
	}
	// T6b: the mode decides how many cards a pick offers (Ultra none: straight on; Insane one, which you must take)
	const int32 NumOffered = CurrentMode().CardsOffered;
	if (NumOffered <= 0)
	{
		return;
	}
	TArray<int32> Free;
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		// Cards held below level III come back as the next level; parked cards (pool false) never show
		// (Thimble isn't offered once the hearts are at their cap)
		const bool bUseless = Cards[Index].Id == TEXT("thimble") && Tuning.Hearts >= Tuning.MaxHearts;
		if (GetCardLevel(Index) < KPDirectorConst::MaxCardLevel && Cards[Index].Pool && !bUseless)
		{
			Free.Add(Index);
		}
	}
	if (Free.Num() == 0)
	{
		return; // every card is taken: straight on
	}
	for (int32 Index = Free.Num() - 1; Index > 0; --Index)
	{
		Free.Swap(Index, FMath::RandRange(0, Index));
	}
	Offer.Reset();
	for (int32 Index = 0; Index < FMath::Min(NumOffered, Free.Num()); ++Index)
	{
		Offer.Add(Free[Index]);
	}
	Mode = EKPMode::Pick;
	TsTarget = 1.f;
	HoveredCard = -1;
	// The tutorial's last prompt, on the first card pick ever (the drawing prompts count as shown by now)
	if (Save && !bAttract && TutorialStep > 0 && TutorialStep < 4)
	{
		Save->TutorialCut = true;
		SaveProgress();
		TutorialStep = 0;
	}
	if (Save && !bAttract && !Save->TutorialPick && NightIndex == 0)
	{
		TutorialStep = 4;
		TutorialFade = 0.f;
	}
}

int32 AKPDirector::CardUnderCursor() const
{
	float MouseX = 0.f;
	float MouseY = 0.f;
	int32 ViewX = 0;
	int32 ViewY = 0;
	if (!PC.IsValid() || !PC->GetMousePosition(MouseX, MouseY))
	{
		return -1;
	}
	PC->GetViewportSize(ViewX, ViewY);
	if (ViewX <= 0 || ViewY <= 0)
	{
		return -1;
	}
	const FVector2D At(MouseX / ViewX, MouseY / ViewY);
	for (int32 Slot = 0; Slot < Offer.Num(); ++Slot)
	{
		if (KPCardLayout::CardRect(Slot, Offer.Num()).IsInside(At))
		{
			return Slot;
		}
	}
	return -1;
}

void AKPDirector::SetLightsOut(bool bOn)
{
	if (bOn == bLightsOut)
	{
		return;
	}
	bLightsOut = bOn;
	if (bOn && !bAttract)
	{
		PlaySound(TEXT("lightsout")); // T7: the switch; the room goes dark on its click (the file is trimmed to start on it)
	}
	if (bOn)
	{
		// Every light in the room drops to a few percent; a warm lamp follows her (UpdateLamp)
		DimmedLights.Reset();
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			TInlineComponentArray<ULightComponent*> Lights(*It);
			for (ULightComponent* Light : Lights)
			{
				if (Light && Light != Lamp)
				{
					DimmedLights.Add({ Light, Light->Intensity });
					Light->SetIntensity(Light->Intensity * Tuning.LightsOutDim);
				}
			}
		}
		const float Cone = FMath::RadiansToDegrees(FMath::Atan2(Tuning.LampRadius, Tuning.LampHeight));
		Lamp->SetIntensityUnits(ELightUnits::Candelas);
		Lamp->SetIntensity(Tuning.LampIntensity);
		Lamp->SetOuterConeAngle(Cone);
		Lamp->SetInnerConeAngle(Cone * 0.7f);
		Lamp->SetAttenuationRadius(Tuning.LampHeight * 2.f);
		Lamp->SetLightColor(FLinearColor(FColor(255, 206, 150)));
		Lamp->SetVisibility(true);
		UpdateLamp();
	}
	else
	{
		for (const TPair<TWeakObjectPtr<ULightComponent>, float>& Entry : DimmedLights)
		{
			if (ULightComponent* Light = Entry.Key.Get())
			{
				Light->SetIntensity(Entry.Value);
			}
		}
		DimmedLights.Reset();
		Lamp->SetVisibility(false);
	}
}

float AKPDirector::LightAt(const FVector& Pos) const
{
	if (!bLightsOut)
	{
		return 1.f;
	}
	// Her lamp's circle, with a soft edge
	const float Distance = FVector::Dist2D(Pos, HeroPos);
	float Light = 1.f - FMath::Clamp((Distance - Tuning.LampRadius + 60.f) / 120.f, 0.f, 1.f);
	// The chalk light: full up to 70% of its reach from the line, nothing past 110%
	if (ChalkLightLevel > 0.f && LitLine.Num() > 0)
	{
		float Nearest = FVector::Dist2D(Pos, LitLine[0]);
		for (int32 Index = 0; Index + 1 < LitLine.Num(); ++Index)
		{
			Nearest = FMath::Min(Nearest, KPMath::SegmentDistance2D(Pos, LitLine[Index], LitLine[Index + 1]));
		}
		const float Reach = FMath::Max(Tuning.ChalkLightReach, 1.f);
		const float Chalk = 1.f - FMath::Clamp((Nearest - Reach * 0.7f) / (Reach * 0.4f), 0.f, 1.f);
		Light = FMath::Max(Light, Chalk * ChalkLightLevel);
	}
	return Light;
}

void AKPDirector::UpdateLamp()
{
	if (bLightsOut && IsWardrobe())
	{
		// T7d: the wardrobe: the lamp in front of her and above, on her face and apron
		const FVector LampAt = HeroPos + FVector(-420.f, 120.f, 760.f);
		Lamp->SetWorldLocationAndRotation(LampAt, (HeroPos + FVector(0.f, 0.f, 120.f) - LampAt).Rotation());
	}
	else if (bLightsOut)
	{
		Lamp->SetWorldLocationAndRotation(HeroPos + FVector(0.f, 0.f, Tuning.LampHeight), FRotator(-90.f, 0.f, 0.f));
	}
	// How far into slow motion we are (matches the HUD's tint): the patchwork glows through it in full colour
	const float Slow = FMath::Clamp((0.7f - Ts) / 0.55f, 0.f, 1.f);
	for (AKPPlushie* Plushie : Plushies)
	{
		if (!IsValid(Plushie))
		{
			continue;
		}
		float Base = Plushie->GetBaseSelfLight();
		if (Plushie->bPatchwork)
		{
			Base = FMath::Lerp(Base, 0.6f, Slow);
		}
		// In the dark only the button eyes glint, a little unevenly
		const float Light = LightAt(Plushie->GetActorLocation());
		Plushie->SetSelfLight(FMath::Lerp(Tuning.LampSilhouette, Base, Light));
		const float Twinkle = 0.75f + 0.25f * FMath::Sin(RealClock * 2.3f + (PTRINT(Plushie) % 97) * 0.37f);
		Plushie->SetEyeGlint(FMath::Lerp(Tuning.EyeGlint * Twinkle, 1.f, Light));
	}
	// Pincushions in the dark can't be seen at all
	for (AKPPincushion* Pin : Pins)
	{
		if (IsValid(Pin))
		{
			Pin->SetLit(LightAt(Pin->GetActorLocation()));
		}
	}
}

void AKPDirector::UpdateChalkLight(float RealDt)
{
	// Lights out: while she draws, the chalk line lights the mat around it; after the cut it fades
	const bool bDrawing = Mode == EKPMode::Draw || Mode == EKPMode::Dash || Mode == EKPMode::Snip;
	if (bLightsOut && bDrawing && Path.Num() > 1)
	{
		LitLine = Path;
		ChalkLightLevel = FMath::Min(1.f, ChalkLightLevel + RealDt * 8.f);
	}
	else
	{
		ChalkLightLevel = FMath::Max(0.f, ChalkLightLevel - RealDt / FMath::Max(Tuning.ChalkLightFade, 0.05f));
	}
	if (!bLightsOut)
	{
		ChalkLightLevel = 0.f;
	}

	// Lights spread evenly along the line
	int32 Used = 0;
	if (ChalkLightLevel > 0.f && LitLine.Num() > 1)
	{
		TArray<float> Along;
		Along.SetNum(LitLine.Num());
		Along[0] = 0.f;
		for (int32 Index = 1; Index < LitLine.Num(); ++Index)
		{
			Along[Index] = Along[Index - 1] + FVector::Dist2D(LitLine[Index - 1], LitLine[Index]);
		}
		const float Total = Along.Last();
		const int32 Max = FMath::Clamp(Tuning.ChalkLights, 1, ChalkLightComps.Num());
		const int32 Count = FMath::Clamp(FMath::CeilToInt(Total / FMath::Max(Tuning.ChalkLightSpacing, 20.f)), 1, Max);
		const float Spacing = Total / Count;
		int32 Segment = 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float S = (Index + 0.5f) * Spacing;
			while (Segment < LitLine.Num() - 2 && Along[Segment + 1] < S)
			{
				++Segment;
			}
			const float Length = FMath::Max(Along[Segment + 1] - Along[Segment], 1e-4f);
			const FVector At = FMath::Lerp(LitLine[Segment], LitLine[Segment + 1], FMath::Clamp((S - Along[Segment]) / Length, 0.f, 1.f));
			UPointLightComponent* Light = ChalkLightComps[Index];
			Light->SetWorldLocation(FVector(At.X, At.Y, KPDirectorConst::ChalkLightHeight));
			// Each light reaches past its neighbours so the band is even; brighter when they're spread thin
			Light->SetAttenuationRadius(FMath::Max(Tuning.ChalkLightReach * 1.25f, Spacing * 0.8f));
			Light->SetIntensity(Tuning.ChalkLightIntensity * ChalkLightLevel);
			Light->SetVisibility(true);
			++Used;
		}
	}
	for (int32 Index = Used; Index < ChalkLightComps.Num(); ++Index)
	{
		if (ChalkLightComps[Index]->IsVisible())
		{
			ChalkLightComps[Index]->SetVisibility(false);
		}
	}
}

void AKPDirector::SeamRip()
{
	// Seam ripper: each snipped plushie throws pins at its neighbours (no chain reaction). They're marked
	// now and count in this snip's combo, but each is only cut when its pins land (UpdateFlyingPins).
	const float Reach = CardValue(TEXT("seamripper"), TEXT("reach"), Tuning.SeamReach)
		+ CardValue(TEXT("seamripper"), TEXT("reachPerLevel"), 75.f) * (FMath::Max(SeamLevel, 1) - 1); // 150, 225, 300
	int32 Ripped = 0;
	for (const AKPPlushie* Source : Marked)
	{
		if (!IsValid(Source))
		{
			continue;
		}
		const FVector From = Source->GetActorLocation();
		for (AKPPlushie* Plushie : Plushies)
		{
			if (!IsValid(Plushie) || Plushie->bMarked || Plushie->bDead || Plushie->bBoss)
			{
				continue; // (Seam ripper pins don't hurt the boss)
			}
			const FVector To = Plushie->GetActorLocation();
			if (FVector::Dist2D(From, To) - Source->Radius - Plushie->Radius > Reach)
			{
				continue;
			}
			// Cut across the pins' flight
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(To.Y - From.Y, To.X - From.X)) + 90.f;
			Plushie->SetMarked(Yaw, 0.f);
			Plushie->bDead = true; // out of the game; sliced when its pins land
			PinTargets.Add(Plushie);
			++Ripped;

			const int32 NumPins = FMath::Max(1, FMath::RoundToInt(CardValue(TEXT("seamripper"), TEXT("pins"), static_cast<float>(Tuning.SeamPinsPerTarget))));
			for (int32 Pin = 0; Pin < NumPins; ++Pin)
			{
				FFlyingPin Flying;
				Flying.From = Source->GetBodyCenter() + FVector(FMath::FRandRange(-0.3f, 0.3f), FMath::FRandRange(-0.3f, 0.3f), FMath::FRandRange(0.f, 0.4f)) * Source->Radius;
				Flying.To = Plushie->GetBodyCenter() + FVector(FMath::FRandRange(-0.35f, 0.35f), FMath::FRandRange(-0.35f, 0.35f), FMath::FRandRange(-0.2f, 0.4f)) * Plushie->Radius;
				Flying.Age = -0.05f * Pin;
				Flying.Duration = Tuning.SeamPinTime * FMath::FRandRange(0.85f, 1.15f);
				Flying.Arc = FMath::FRandRange(90.f, 170.f);
				Flying.bLast = Pin == NumPins - 1;
				Flying.ColorIndex = FMath::RandRange(0, KPDirectorConst::NumPinHeadColors - 1);
				Flying.Target = Plushie;
				FlyingPins.Add(Flying);
			}
		}
	}
	if (Ripped > 0)
	{
		UE_LOG(LogKP, Log, TEXT("Seam ripper: %d more cut"), Ripped);
	}
}

void AKPDirector::UpdateFlyingPins(float RealDt)
{
	TArray<FTransform> Shafts;
	TArray<FTransform> HeadsByColor[KPDirectorConst::NumPinHeadColors];
	for (int32 Index = FlyingPins.Num() - 1; Index >= 0; --Index)
	{
		FFlyingPin& Pin = FlyingPins[Index];
		Pin.Age += RealDt;
		if (Pin.Age >= Pin.Duration)
		{
			// A small hit: a puff, a twitch of the camera; the pin stays stuck in it (and in its half after
			// the cut, so you see afterwards what got pinned), and the last pin cuts it
			Burst(Pin.To, 4, FVector::ZeroVector, false);
			Shake = FMath::Max(Shake, 3.f);
			PlaySound(TEXT("pin")); // T7 (minGap keeps a volley from stacking)
			AKPPlushie* Target = Pin.Target.Get();
			if (IsValid(Target))
			{
				const FVector Arrive = ((Pin.To - Pin.From) + FVector(0.f, 0.f, -Pin.Arc * 4.f)).GetSafeNormal();
				// The tip goes a third of the way into the body
				const FVector Center = Target->GetBodyCenter();
				const FVector Entry = Center - Arrive * Target->Radius * 0.85f;
				Target->AddStuckPin(Entry + Arrive * (KPDirectorConst::SeamPinLength * 0.3f), Arrive, KPDirectorConst::SeamPinLength,
					KPVisuals::Hex(KPDirectorConst::PinHeadColors[FMath::Clamp(Pin.ColorIndex, 0, KPDirectorConst::NumPinHeadColors - 1)]));
			}
			if (Pin.bLast && IsValid(Target) && PinTargets.Contains(Target))
			{
				PinTargets.Remove(Target);
				SliceQueue.Add(Target);
				AddWorldText(Target->GetActorLocation() + FVector(0.f, 0.f, 140.f), TEXT("Ripped!"), FString(), 0.8f, 12.f, FLinearColor::White);
			}
			FlyingPins.RemoveAt(Index);
			continue;
		}
		if (Pin.Age < 0.f)
		{
			continue;
		}
		// Along an arc, point first
		const float T = Pin.Age / FMath::Max(Pin.Duration, 0.01f);
		const FVector Pos = FMath::Lerp(Pin.From, Pin.To, T) + FVector(0.f, 0.f, Pin.Arc * 4.f * T * (1.f - T));
		const FVector Velocity = (Pin.To - Pin.From) + FVector(0.f, 0.f, Pin.Arc * 4.f * (1.f - 2.f * T));
		const FVector Dir = Velocity.GetSafeNormal();
		// Big enough to read from the high camera (a real pin would be a few pixels)
		constexpr float Length = KPDirectorConst::SeamPinLength;
		Shafts.Add(FTransform(FRotationMatrix::MakeFromZ(Dir).Rotator(), Pos, FVector(0.11f, 0.11f, Length / 100.f)));
		HeadsByColor[FMath::Clamp(Pin.ColorIndex, 0, KPDirectorConst::NumPinHeadColors - 1)].Add(FTransform(FRotator::ZeroRotator, Pos - Dir * (Length * 0.5f), FVector(0.36f)));
		PinTrail.Add({ Pos - Dir * (Length * 0.5f), KPDirectorConst::PinTrailLife });
	}
	// The trails: bright dots left behind each head, shrinking away
	TArray<FTransform> TrailDots;
	for (int32 Index = PinTrail.Num() - 1; Index >= 0; --Index)
	{
		PinTrail[Index].Life -= RealDt;
		if (PinTrail[Index].Life <= 0.f)
		{
			PinTrail.RemoveAtSwap(Index);
			continue;
		}
		TrailDots.Add(FTransform(FRotator::ZeroRotator, PinTrail[Index].Pos, FVector(0.26f * PinTrail[Index].Life / KPDirectorConst::PinTrailLife)));
	}
	SetInstances(PinTrailIsm, TrailDots);
	// Pins whose plushie went away (a restart) don't hold the wave up
	for (int32 Index = PinTargets.Num() - 1; Index >= 0; --Index)
	{
		AKPPlushie* Target = PinTargets[Index];
		const bool bHasPin = FlyingPins.ContainsByPredicate([Target](const FFlyingPin& Pin) { return Pin.Target.Get() == Target; });
		if (!bHasPin)
		{
			PinTargets.RemoveAt(Index);
			if (IsValid(Target))
			{
				SliceQueue.Add(Target);
			}
		}
	}
	SetInstances(FlyPinIsm, Shafts);
	for (int32 Index = 0; Index < FlyPinHeadIsms.Num(); ++Index)
	{
		SetInstances(FlyPinHeadIsms[Index], HeadsByColor[Index]);
	}
}

bool AKPDirector::ExtraInchCone(FVector& OutTip, FVector& OutDir, float& OutHalfAngle, float& OutLength) const
{
	// Ahead of the line's end, along its last direction (on Double stitch's way back, still the far end)
	if (!bExtraInch || ExtraInchLevel <= 0 || Path.Num() < 2)
	{
		return false;
	}
	const bool bBack = bReturning && Mode == EKPMode::Dash;
	const FVector End = bBack ? Path[0] : Path.Last();
	// The last direction: from a point some way back along the line, so a wobble at the end doesn't swing it
	FVector Back = End;
	float Walked = 0.f;
	for (int32 Step = 1; Step < Path.Num() && Walked < 60.f; ++Step)
	{
		const FVector Prev = bBack ? Path[Step] : Path[Path.Num() - 1 - Step];
		Walked += FVector::Dist2D(Back, Prev);
		Back = Prev;
	}
	FVector Dir = End - Back;
	Dir.Z = 0.f;
	if (!Dir.Normalize())
	{
		return false;
	}
	const int32 Level = FMath::Clamp(ExtraInchLevel, 1, 3) - 1;
	static const float DefaultAngles[3] = { 30.f, 50.f, 70.f };
	static const float DefaultLengths[3] = { 70.f, 100.f, 130.f };
	// The card's values: angle1/length1 for level I, angle2/length2, angle3/length3
	const float Angle = CardValue(TEXT("extrainch"), *FString::Printf(TEXT("angle%d"), Level + 1), DefaultAngles[Level]);
	OutLength = CardValue(TEXT("extrainch"), *FString::Printf(TEXT("length%d"), Level + 1), DefaultLengths[Level]);
	OutHalfAngle = FMath::DegreesToRadians(Angle * 0.5f);
	OutTip = FVector(End.X, End.Y, 0.f);
	OutDir = Dir;
	return true;
}

void AKPDirector::ExtraInchTargets(const FVector& End, TArray<AKPPlushie*>& Out) const
{
	FVector Tip;
	FVector Dir;
	float Half = 0.f;
	float Length = 0.f;
	if (!ExtraInchCone(Tip, Dir, Half, Length))
	{
		return;
	}
	for (AKPPlushie* Plushie : Plushies)
	{
		if (!IsValid(Plushie) || Plushie->bDead)
		{
			continue;
		}
		// Its body touches the cone: within reach, and inside the angle widened by its own size
		FVector To = Plushie->GetActorLocation() - Tip;
		To.Z = 0.f;
		const float Distance = To.Size();
		if (Distance <= Plushie->Radius)
		{
			Out.Add(Plushie);
			continue;
		}
		if (Distance - Plushie->Radius > Length)
		{
			continue;
		}
		const float Off = FMath::Acos(FMath::Clamp(static_cast<float>(FVector::DotProduct(To / Distance, Dir)), -1.f, 1.f));
		if (Off <= Half + FMath::Asin(FMath::Clamp(Plushie->Radius / Distance, 0.f, 1.f)))
		{
			Out.Add(Plushie);
		}
	}
}

AKPPincushion* AKPDirector::MagnetTarget() const
{
	if (MagnetLeft <= 0 || Path.Num() < 2)
	{
		return nullptr;
	}
	// In drawing order: the first pincushion the line reaches
	for (int32 Index = 0; Index + 1 < Path.Num(); ++Index)
	{
		AKPPincushion* FirstPin = nullptr;
		float NearestAlong = TNumericLimits<float>::Max();
		for (AKPPincushion* Pin : Pins)
		{
			if (!IsValid(Pin) || Pin->bHit)
			{
				continue;
			}
			const FVector Location = Pin->GetActorLocation();
			if (KPMath::SegmentDistance2D(Location, Path[Index], Path[Index + 1]) <= Pin->Radius + Tuning.BladePad)
			{
				const float Along = FVector::Dist2D(Path[Index], KPMath::ClosestPointOnSegment2D(Location, Path[Index], Path[Index + 1]));
				if (Along < NearestAlong)
				{
					NearestAlong = Along;
					FirstPin = Pin;
				}
			}
		}
		if (FirstPin)
		{
			return FirstPin;
		}
	}
	return nullptr;
}

void AKPDirector::ApplyExtraInch()
{
	// Extra inch: the end of the line also snips everything touching a small circle around it
	if (Path.Num() < 2)
	{
		return;
	}
	const FVector End = Path.Last();
	const FVector Before = Path[Path.Num() - 2];
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(End.Y - Before.Y, End.X - Before.X));
	const FVector Normal = KPMath::CutNormal(Yaw);
	TArray<AKPPlushie*> Caught;
	ExtraInchTargets(End, Caught);
	for (AKPPlushie* Plushie : Caught)
	{
		if (Plushie->bMarked)
		{
			continue;
		}
		const FVector Center = Plushie->GetActorLocation();
		const float Offset = static_cast<float>(FVector::DotProduct(FVector(End.X - Center.X, End.Y - Center.Y, 0.f), Normal));
		Plushie->SetMarked(Yaw, Offset);
		Plushie->CutDistance = FVector::Dist2D(Center, End);
		Marked.Add(Plushie);
	}
	Burst(End + FVector(0.f, 0.f, 20.f), 10, FVector::ZeroVector, false);
}

void AKPDirector::ApplyCut(bool bAnnounce)
{
	const bool bTheatrical = KPSettings::IsTheatrical();
	// Halves already on the mat change their insides too
	for (AKPPlushie* Plushie : CutPlushies)
	{
		if (IsValid(Plushie))
		{
			Plushie->SetCapTheatrical(bTheatrical);
		}
	}
	if (bTheatrical)
	{
		// No blood anywhere: wipe the mat and drop the blood still in the air
		StainIsm->ClearInstances();
		StainCursor = 0;
		SplatIsm->ClearInstances();
		SplatCursor = 0;
		Particles.RemoveAll([](const FParticle& Particle) { return Particle.Kind != Fluff; });
	}
	if (bAnnounce)
	{
		AddScreenText(FVector2D(0.5, 0.3), FString(bTheatrical ? TEXT("Theatrical cut") : TEXT("Director's cut")), 1.6f, 18.f, FLinearColor::White);
	}
}

void AKPDirector::JumpToWave(int32 WaveNumber)
{
	if (Mode == EKPMode::Over || Mode == EKPMode::Dawn)
	{
		ResetRun();
	}
	ClearPlushies();
	Marked.Reset();
	Path.Reset();
	PathLength = 0.f;
	Offer.Reset();
	IntroQueue.Reset();
	if (Portrait)
	{
		Portrait->Clear();
	}
	Mode = EKPMode::Play;
	TsTarget = 1.f;
	Queue = 0;
	SpawnList.Reset();
	PathPinsLeft = 0;
	ImpostorTimer = -1.f;
	if (IsEndless())
	{
		EnsureEndlessRound(WaveNumber);
	}
	Wave = FMath::Clamp(WaveNumber, 1, GetNumWaves()) - 1;
	bBreakDone = true;
	WaveTimer = 0.f;
	Countdown = 0.f;
}

void AKPDirector::ClearPlushies()
{
	for (AKPPlushie* Plushie : Plushies)
	{
		if (IsValid(Plushie))
		{
			Plushie->Destroy();
		}
	}
	Plushies.Reset();
	for (AKPPlushie* Plushie : CutPlushies)
	{
		if (IsValid(Plushie))
		{
			Plushie->Destroy();
		}
	}
	CutPlushies.Reset();
	for (AKPPlushie* Plushie : SliceQueue)
	{
		if (IsValid(Plushie))
		{
			Plushie->Destroy();
		}
	}
	SliceQueue.Reset();
	for (AKPPlushie* Plushie : PinTargets)
	{
		if (IsValid(Plushie))
		{
			Plushie->Destroy();
		}
	}
	PinTargets.Reset();
	FlyingPins.Reset();
	for (AKPPincushion* Pin : Pins)
	{
		if (IsValid(Pin))
		{
			Pin->Destroy();
		}
	}
	Pins.Reset();
}

void AKPDirector::UpdatePieces(float GameDt)
{
	ProcessSliceQueue();

	// Over the cap (two halves per plushie): the oldest start shrinking now
	const int32 MaxCut = FMath::Max(1, Tuning.MaxPieces / 2);
	for (int32 Index = 0; Index < CutPlushies.Num() - MaxCut; ++Index)
	{
		if (IsValid(CutPlushies[Index]))
		{
			CutPlushies[Index]->PieceLife = FMath::Min(CutPlushies[Index]->PieceLife, KPDirectorConst::PieceShrinkTime);
		}
	}

	for (int32 Index = CutPlushies.Num() - 1; Index >= 0; --Index)
	{
		AKPPlushie* Plushie = CutPlushies[Index];
		if (IsValid(Plushie))
		{
			Plushie->PieceLife -= GameDt;
			if (Plushie->PieceLife > 0.f)
			{
				if (Plushie->PieceLife < KPDirectorConst::PieceShrinkTime)
				{
					Plushie->SetPiecesScale(Plushie->PieceLife / KPDirectorConst::PieceShrinkTime);
				}
				continue;
			}
			Plushie->Destroy();
		}
		CutPlushies.RemoveAt(Index);
	}
}

bool AKPDirector::EnsureWorldRefs()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (!PC.IsValid())
	{
		PC = Cast<AKPPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
		if (!PC.IsValid())
		{
			return false;
		}
	}

	if (!Hero.IsValid())
	{
		Hero = Cast<AKPHeroine>(PC->GetPawn());
		if (!Hero.IsValid())
		{
			TActorIterator<AKPHeroine> It(World);
			if (It)
			{
				Hero = *It;
			}
		}
		static int32 FramesWithoutHero = 0;
		if (!Hero.IsValid() && ++FramesWithoutHero > KPDirectorConst::HeroWaitFrames)
		{
			// The game mode should have spawned her; make one so the game still runs
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Hero = World->SpawnActor<AKPHeroine>(AKPHeroine::StaticClass(), HeroPos, FRotator(0.f, HeroYaw, 0.f), Params);
			FramesWithoutHero = 0;
		}
		if (!Hero.IsValid())
		{
			return false;
		}
		Hero->PlayIdle();
	}

	if (!Camera.IsValid())
	{
		for (TActorIterator<ACameraActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(KPDirectorConst::CameraTag))
			{
				Camera = *It;
				break;
			}
		}
		if (!Camera.IsValid())
		{
			UE_LOG(LogKP, Warning, TEXT("No CameraActor tagged KPCamera in the level, using a default camera"));
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACameraActor* NewCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), KPDirectorConst::CameraLocation, KPDirectorConst::CameraRotation, Params);
			if (NewCamera)
			{
				NewCamera->GetCameraComponent()->SetFieldOfView(KPDirectorConst::CameraFov);
			}
			Camera = NewCamera;
		}
		if (!Camera.IsValid())
		{
			return false;
		}
		Camera->GetCameraComponent()->SetConstraintAspectRatio(false);
		CameraBase = Camera->GetActorTransform();
	}

	if (!CineCam.IsValid())
	{
		// T7d: the close camera (camera cuts, the wardrobe), with the game camera's post process (the fixed exposure)
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ACameraActor* NewCam = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraBase.GetLocation(), CameraBase.Rotator(), Params);
		if (NewCam)
		{
			UCameraComponent* From = Camera->GetCameraComponent();
			UCameraComponent* To = NewCam->GetCameraComponent();
			To->PostProcessSettings = From->PostProcessSettings;
			To->PostProcessBlendWeight = From->PostProcessBlendWeight;
			To->SetConstraintAspectRatio(false);
			To->SetFieldOfView(KPDirectorConst::CineFov);
		}
		CineCam = NewCam;
	}

	const bool bCine = CineCam.IsValid() && (CineMode != 0 || bCloseUpTest || IsWardrobe());
	AActor* Wanted = bCine ? static_cast<AActor*>(CineCam.Get()) : static_cast<AActor*>(Camera.Get());
	if (PC->GetViewTarget() != Wanted)
	{
		PC->SetViewTarget(Wanted);
		bCineSnap = true;
	}
	return true;
}

// ---------------------------------------------------------------------------
// The loop

void AKPDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!EnsureWorldRefs())
	{
		return;
	}
	if (!HudHost.IsValid())
	{
		CreateHud();
	}

	// Real (undilated) frame time, clamped like the toy so a hitch doesn't teleport anything
	const float Dt = FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 1.f / 30.f);

	// T7: the sound test panel takes its clicks first (the press is then used up)
	if (bSoundTest && PC.IsValid())
	{
		const bool bHeldNow = PC->IsCutHeld();
		if (bHeldNow && !bWasHeld)
		{
			const FString Action = UiActionUnderCursor();
			if (Action.StartsWith(TEXT("st:")))
			{
				HandleSoundTestAction(Action);
				bWasHeld = true;
			}
		}
	}

	// Pause: the world stops (time scale kept); only the menu runs
	UpdatePauseInput();
	UpdateAudio(Dt); // T7 (also while paused: the volume arrows, the music)
	const float BlurTarget = (bPaused || Mode == EKPMode::Intro) ? 1.f : 0.f;
	BlurAmount = FMath::FInterpConstantTo(BlurAmount, BlurTarget, Dt, 5.f);
	if (bPaused)
	{
		UiClock += Dt;
		RealClock += Dt;
		return;
	}

	UpdateInput();
	UpdateTutorial(Dt);
	if (bAttract)
	{
		UpdateTitleAuto(Dt);
	}
	if (!bWardrobeApplied)
	{
		ApplyWardrobe();
	}
	if (IsWardrobe())
	{
		// T7d: the wardrobe: she stands in the lamp light, shears open, a ring of her dash trail round her feet
		for (int32 Dot = 0; Dot < 3; ++Dot)
		{
			const float Angle = RealClock * 5.f + Dot * 0.07f;
			Trail.Add({ HeroPos + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 150.f, KPDirectorConst::TrailLife });
		}
	}
	else if (bAutoCut || bAttract)
	{
		UpdateAutoCut(Dt);
	}

	// The boss's final split: slow motion held a little longer
	if (FinaleTimer > 0.f)
	{
		FinaleTimer -= Dt;
		TsTarget = FinaleTimer > 0.f ? Tuning.DashTimeScale : 1.f;
	}
	Ts += (TsTarget - Ts) * FMath::Min(1.f, Dt * Tuning.TimeScaleRate);
	// T7b: hitstop: the world holds still for a few frames after a snip (the camera shake and the UI run on)
	float WorldTs = Ts;
	if (HitStopTimer > 0.f)
	{
		HitStopTimer -= Dt;
		WorldTs = FMath::Min(Ts, FMath::Max(0.0001f, Tuning.HitStopScale));
	}
	UGameplayStatics::SetGlobalTimeDilation(this, WorldTs);
	const float G = Dt * WorldTs;

	Shake = FMath::Max(0.f, Shake - Dt * 45.f);
	Flash -= Dt;
	Invulnerable -= Dt;
	Cooldown -= Dt;

	RealClock += Dt;
	UiClock += Dt;
	for (int32 Index = ShotTimes.Num() - 1; Index >= 0; --Index)
	{
		if (RealClock >= ShotTimes[Index])
		{
			const FString File = FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/KP/Auto_at%02d.png"), FMath::RoundToInt(ShotTimes[Index]));
			FScreenshotRequest::RequestScreenshot(File, true, false);
			ShotTimes.RemoveAt(Index);
		}
	}
	if (Countdown > 0.f && Mode == EKPMode::Play && !bAttract)
	{
		Countdown = FMath::Max(0.f, Countdown - Dt); // T6c: the 3-2-1 (no waves meanwhile)
	}
	else if (!bAttract && (Mode == EKPMode::Play || Mode == EKPMode::Draw || Mode == EKPMode::Dash || Mode == EKPMode::Snip))
	{
		// T6d: active play time, in real seconds (drawing too; not the countdown, picks, intros or results)
		PlayTime += FApp::GetDeltaTime();
	}
	if (Mode != EKPMode::Over)
	{
		if (bAttract)
		{
			if (!IsWardrobe())
			{
				UpdateAttract(G);
			}
		}
		else if (Countdown <= 0.f)
		{
			UpdateWaves(G, Dt);
		}
		UpdatePlushies(G, Dt);
		LayEggs(G);
		UpdatePins(G);
		if (Mode == EKPMode::Dash)
		{
			UpdateDash(Dt);
		}
		else if (Mode == EKPMode::Snip)
		{
			SnipTimer -= Dt;
			if (SnipTimer <= 0.f)
			{
				DoSnip();
			}
		}
	}

	UpdateFx(Dt, G);
	UpdateFlyingPins(Dt);
	UpdateSplits(G);
	UpdatePieces(G);
	UpdateChalkLight(Dt);
	UpdateLamp();
	UpdatePreview();
	UpdateLineVisuals();
	UpdateHero(Dt);
	UpdateCamera();
}

void AKPDirector::UpdateInput()
{
	const bool bHeld = PC->IsCutHeld();
	const bool bPressed = bHeld && !bWasHeld;
	const bool bReleased = !bHeld && bWasHeld;
	bWasHeld = bHeld;

	FVector Cursor;
	const bool bHasCursor = PC->GetCursorOnPlane(0.f, Cursor);

	// The title screen: clicks go to its menu (the mat behind it plays itself)
	if (bAttract)
	{
		if (bPressed)
		{
			HandleUiAction(UiActionUnderCursor());
		}
		return;
	}

	if (Mode == EKPMode::Intro)
	{
		// Meet a new plushie: a click goes on (after a moment, so a cut's click can't skip it)
		if (bPressed && UiClock >= Tuning.UiClickDelay)
		{
			PlaySound(TEXT("click"));
			FinishIntro();
		}
		return;
	}
	if (Mode == EKPMode::Pick)
	{
		const int32 Hovered = CardUnderCursor();
		if (Hovered != HoveredCard && Offer.IsValidIndex(Hovered))
		{
			PlaySound(TEXT("cardhover")); // T7: the mouse moves onto a card
		}
		HoveredCard = Hovered;
		if (bPressed && Offer.IsValidIndex(HoveredCard))
		{
			PlaySound(TEXT("click"));
			TakeCard(Cards[Offer[HoveredCard]].Id);
		}
		return;
	}
	// The results screen after a night (won or lost): its button is painted by the HUD
	if (Mode == EKPMode::Dawn || Mode == EKPMode::Over)
	{
		if (bPressed)
		{
			HandleUiAction(UiActionUnderCursor());
		}
		return;
	}

	if (bPressed)
	{
		if (Mode == EKPMode::Play && Cooldown <= 0.f && bHasCursor)
		{
			Mode = EKPMode::Draw;
			TsTarget = Tuning.DrawTimeScale;
			Path.Reset();
			Path.Add(HeroPos);
			PathLength = 0.f;
			bHint = false;
			AddPathPoint(Cursor);
		}
	}
	else if (bHeld && Mode == EKPMode::Draw && bHasCursor)
	{
		AddPathPoint(Cursor);
	}

	if (bReleased && Mode == EKPMode::Draw)
	{
		if (PathLength < Tuning.MinLine)
		{
			// Too short: cancel
			Mode = EKPMode::Play;
			TsTarget = 1.f;
			Path.Reset();
			PathLength = 0.f;
		}
		else
		{
			StartDash();
		}
	}
}

void AKPDirector::StartDash()
{
	Mode = EKPMode::Dash;
	DashIndex = 0;
	DashS = 0.f;
	DashTime = 0.f;
	bReturning = false;
	Marked.Reset();
	TsTarget = Tuning.DashTimeScale;
	PlaySound(TEXT("whoosh"));
	// T7d: a line through 5 or more: the camera cuts to her for the dash and the beat
	if (CountOnPath() >= FMath::Max(1, Tuning.CameraCutAt))
	{
		StartCine(1);
	}
	if (AKPHeroine* H = Hero.Get())
	{
		H->PlayDash();
	}
	if (bAutoCut && AutoShotsLeft > 0 && AutoShotTimer <= 0.f && FParse::Param(FCommandLine::Get(), TEXT("KPDashShots")))
	{
		AutoShotTimer = 0.07f; // T7f test: -KPDashShots screenshots her mid-dash
	}
}

namespace KPDrawShotTest
{
	// T8 test (-KPDrawShots): the bot's line drawn out slowly, for screenshots and the itch GIF
	bool bActive = false;
	float T = 0.f;
	const float Dur = 1.6f;
	const float Hold = 0.7f;
	float FullLength = 0.f;
	TArray<FVector> Full;
}

void AKPDirector::UpdateAutoCut(float RealDt)
{
	// Screenshots a moment after a snip, for checking a build nobody is watching
	if (AutoShotTimer > 0.f)
	{
		AutoShotTimer -= RealDt;
		if (AutoShotTimer <= 0.f && AutoShotsLeft > 0)
		{
			--AutoShotsLeft;
			const FString File = FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/KP/Auto_%02d.png"), ++AutoShotIndex);
			FScreenshotRequest::RequestScreenshot(File, true, false);
		}
	}

	if (Mode == EKPMode::Over || Mode == EKPMode::Dawn)
	{
		UpdateResultsAuto(RealDt);
		return;
	}
	if (Mode == EKPMode::Intro)
	{
		// A new plushie's introduction: a screenshot (if any are left), then click on
		if (UiClock >= 1.2f && UiClock - RealDt < 1.2f && AutoShotsLeft > 0)
		{
			--AutoShotsLeft;
			const FString File = FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/KP/Auto_%02d_meet_%s.png"), ++AutoShotIndex, *IntroKey);
			FScreenshotRequest::RequestScreenshot(File, true, false);
		}
		if (UiClock >= 2.f)
		{
			FinishIntro();
		}
		return;
	}
	if (Mode == EKPMode::Pick && Offer.Num() > 0)
	{
		// Wait a moment (so the pick can be seen and screenshotted), then take a card
		if (AutoPickTimer <= 0.f)
		{
			AutoPickTimer = 1.2f;
			if (AutoShotsLeft > 0)
			{
				AutoShotTimer = 0.6f;
			}
		}
		AutoPickTimer -= RealDt;
		if (AutoPickTimer <= 0.f)
		{
			TakeCard(Cards[Offer[FMath::RandRange(0, Offer.Num() - 1)]].Id);
		}
		return;
	}
	if (KPDrawShotTest::bActive && Mode == EKPMode::Draw)
	{
		// T8 test (-KPDrawShots): the planned line is drawn out over Dur real seconds in slow motion, held, then cut
		KPDrawShotTest::T += RealDt;
		const float Reveal = FMath::Clamp(KPDrawShotTest::T / KPDrawShotTest::Dur, 0.f, 1.f) * KPDrawShotTest::FullLength;
		const TArray<FVector>& Full = KPDrawShotTest::Full;
		Path.Reset();
		Path.Add(Full[0]);
		PathLength = 0.f;
		for (int32 Index = 1; Index < Full.Num() && PathLength < Reveal; ++Index)
		{
			const float Segment = FVector::Dist2D(Full[Index - 1], Full[Index]);
			const float Take = FMath::Min(Segment, Reveal - PathLength);
			Path.Add(Segment > 0.f ? FMath::Lerp(Full[Index - 1], Full[Index], Take / Segment) : Full[Index]);
			PathLength += Take;
		}
		if (KPDrawShotTest::T >= KPDrawShotTest::Dur + KPDrawShotTest::Hold)
		{
			KPDrawShotTest::bActive = false;
			Path = Full;
			PathLength = KPDrawShotTest::FullLength;
			StartDash();
		}
		return;
	}
	if (Mode != EKPMode::Play || Cooldown > 0.f)
	{
		return;
	}
	AutoCutTimer -= RealDt;
	if (AutoCutTimer > 0.f)
	{
		return;
	}
	AutoCutTimer = bAttract ? Tuning.TitleCutEvery * FMath::FRandRange(0.8f, 1.2f) : 0.9f;

	// From her through the nearest plushie, then the nearest to that, for as long as the chalk lasts
	const float LimitX = Tuning.ArenaHalfX - Tuning.EdgeMargin;
	const float LimitY = Tuning.ArenaHalfY - Tuning.EdgeMargin;
	TArray<FVector> Targets;
	const AKPPlushie* BossTarget = bAutoBossOnly && BossPlushie.IsValid() && !BossPlushie->bDead ? BossPlushie.Get() : nullptr;
	for (const AKPPlushie* Plushie : Plushies)
	{
		if (IsValid(Plushie) && !Plushie->bDead && (!BossTarget || Plushie == BossTarget))
		{
			const FVector Location = Plushie->GetActorLocation();
			if (FMath::Abs(Location.X) < LimitX && FMath::Abs(Location.Y) < LimitY)
			{
				Targets.Add(FVector(Location.X, Location.Y, 0.f));
			}
		}
	}
	const int32 MinTargets = (!bAttract && FParse::Param(FCommandLine::Get(), TEXT("KPDrawShots"))) ? 6 : 3; // (T8: big lines for the itch shots)
	if (Targets.Num() == 0 || (!BossTarget && Targets.Num() < MinTargets && (Queue > 0 || bAttract)))
	{
		AutoCutTimer = bAttract ? 0.3f : AutoCutTimer;
		return; // wait for more of the wave
	}

	Path.Reset();
	Path.Add(HeroPos);
	PathLength = 0.f;
	FVector From = HeroPos;
	while (Targets.Num() > 0)
	{
		int32 Nearest = 0;
		for (int32 Index = 1; Index < Targets.Num(); ++Index)
		{
			if (FVector::DistSquared2D(Targets[Index], From) < FVector::DistSquared2D(Targets[Nearest], From))
			{
				Nearest = Index;
			}
		}
		const float Distance = FVector::Dist2D(Targets[Nearest], From);
		if (PathLength + Distance > Tuning.Chalk)
		{
			break;
		}
		From = Targets[Nearest];
		Path.Add(From);
		PathLength += Distance;
		Targets.RemoveAtSwap(Nearest);
	}
	// T6b: a line that ends in the boss carries on out the other side (a player wouldn't stop inside it)
	const AKPPlushie* LiveBoss = BossPlushie.IsValid() && !BossPlushie->bDead ? BossPlushie.Get() : nullptr;
	if (LiveBoss && Path.Num() >= 2)
	{
		const FVector BossAt(LiveBoss->GetActorLocation().X, LiveBoss->GetActorLocation().Y, 0.f);
		const float Clear = LiveBoss->Radius + Tuning.HeroRadius + 80.f;
		if (FVector::Dist2D(Path.Last(), BossAt) < Clear)
		{
			const FVector Dir = (Path.Last() - Path[Path.Num() - 2]).GetSafeNormal2D();
			const float Need = Clear - FVector::DotProduct(Path.Last() - BossAt, Dir);
			const float Room = FMath::Max(0.f, Tuning.Chalk - PathLength - 1.f);
			const FVector End = Path.Last() + Dir * FMath::Min(Need, Room);
			const float LimitX2 = Tuning.ArenaHalfX - Tuning.EdgeMargin;
			const float LimitY2 = Tuning.ArenaHalfY - Tuning.EdgeMargin;
			const FVector Clamped(FMath::Clamp(End.X, -LimitX2, LimitX2), FMath::Clamp(End.Y, -LimitY2, LimitY2), 0.f);
			PathLength += FVector::Dist2D(Path.Last(), Clamped);
			Path.Add(Clamped);
		}
	}
	if (Path.Num() < 2 || PathLength < Tuning.MinLine)
	{
		Path.Reset();
		PathLength = 0.f;
		return;
	}
	bHint = false;
	if (!bAttract && Path.Num() - 1 >= 4 && FParse::Param(FCommandLine::Get(), TEXT("KPDrawShots")))
	{
		// T8 test: draw this line visibly (a screenshot near its end, if any are left), then cut
		KPDrawShotTest::Full = Path;
		KPDrawShotTest::FullLength = PathLength;
		KPDrawShotTest::T = 0.f;
		KPDrawShotTest::bActive = true;
		Mode = EKPMode::Draw;
		TsTarget = Tuning.DrawTimeScale;
		Path.SetNum(1);
		PathLength = 0.f;
		if (AutoShotsLeft > 0)
		{
			AutoShotTimer = KPDrawShotTest::Dur + 0.25f;
		}
		return;
	}
	StartDash();
}

void AKPDirector::AddPathPoint(FVector Point)
{
	const FVector Last = Path.Last();
	FVector Delta = Point - Last;
	Delta.Z = 0.f;
	float Distance = Delta.Size();
	if (Distance < Tuning.PointSpacing)
	{
		return;
	}
	const float Remaining = Tuning.Chalk - PathLength;
	if (Remaining <= 1.f)
	{
		return;
	}
	if (Distance > Remaining)
	{
		Point = Last + Delta / Distance * Remaining;
		Distance = Remaining;
	}
	const float LimitX = Tuning.ArenaHalfX - Tuning.EdgeMargin;
	const float LimitY = Tuning.ArenaHalfY - Tuning.EdgeMargin;
	Point.X = FMath::Clamp(Point.X, -LimitX, LimitX);
	Point.Y = FMath::Clamp(Point.Y, -LimitY, LimitY);
	Point.Z = 0.f;
	Path.Add(Point);
	PathLength += Distance;
}

void AKPDirector::UpdateWaves(float GameDt, float RealDt)
{
	UpdateBoss(GameDt, RealDt);

	// T7d: the impostor turns up a few seconds into its wave (only one at a time)
	if (ImpostorTimer >= 0.f && !bBreakDone)
	{
		ImpostorTimer -= GameDt;
		const bool bOneAlive = Plushies.ContainsByPredicate([](const AKPPlushie* Each) { return IsValid(Each) && Each->bImpostor && !Each->bDead; });
		if (ImpostorTimer <= 0.f)
		{
			ImpostorTimer = (!bOneAlive && SpawnImpostor()) ? -1.f : 0.5f;
		}
	}

	// Pincushions placed in her way, once some of the wave is on the mat
	if (PathPinsLeft > 0 && !bBreakDone)
	{
		PathPinTimer -= GameDt;
		if (PathPinTimer <= 0.f)
		{
			if (SpawnPinInPath())
			{
				--PathPinsLeft;
				PathPinTimer = Tuning.PinPathEvery;
			}
			else
			{
				PathPinTimer = 0.4f; // nobody close enough yet: try again soon
			}
		}
	}

	if (SpawnList.Num() > 0)
	{
		SpawnTimer -= GameDt;
		if (SpawnTimer <= 0.f && AliveCount() < Tuning.AliveCap)
		{
			const int32 TypeIndex = SpawnList[0];
			SpawnList.RemoveAt(0);
			SpawnPlushie(TypeIndex >= 0 ? TypeIndex : PickType());
			SpawnTimer = CurrentWave().Interval;
		}
		Queue = SpawnList.Num();
		return;
	}
	Queue = 0;
	const bool bClear = Plushies.Num() == 0 && SliceQueue.Num() == 0 && PinTargets.Num() == 0 && !HasPendingSplits() && ImpostorTimer < 0.f;
	if (!bClear || Mode != EKPMode::Play || FinaleTimer > 0.f)
	{
		return;
	}
	if (!bBreakDone)
	{
		// The wave is cleared: its pincushions shrink away, a moment to breathe, then a card pick,
		// or dawn after the night's last wave
		if (!bWaveClearSounded)
		{
			bWaveClearSounded = true;
			PlaySound(TEXT("waveclear")); // T7
			UE_LOG(LogKP, Log, TEXT("Wave %d cleared: %d plushies spawned, %d golden (%d from the golden roll)"), Wave, WaveSpawned, WaveGolden, WaveGoldenRolled);
			WaveSpawned = 0;
			WaveGolden = 0;
			WaveGoldenRolled = 0;
		}
		PathPinsLeft = 0;
		FadeOutPins();
		if (bLightsOut && !CurrentNight().Rule.Equals(TEXT("lightsout"), ESearchCase::IgnoreCase))
		{
			SetLightsOut(false); // T6c: a dark wave is over
		}
		BreakTimer -= RealDt;
		if (BreakTimer > 0.f)
		{
			return;
		}
		bBreakDone = true;
		WaveTimer = Tuning.WaveDelay;
		if (IsEndless())
		{
			// T6c: no dawn in Extended cut: a card pick every cardEvery rounds, otherwise straight on
			if (Endless.CardEvery > 0 && Wave % Endless.CardEvery == 0)
			{
				OpenPick();
			}
			return;
		}
		if (Wave >= GetNumWaves())
		{
			Mode = EKPMode::Dawn;
			SetLightsOut(false); // it's morning
			UE_LOG(LogKP, Log, TEXT("Dawn: %s survived, score %d"), *CurrentNight().Name, Score);
			RecordNightEnd(true);
			return;
		}
		OpenPick();
		return;
	}
	WaveTimer -= RealDt;
	if (WaveTimer <= 0.f)
	{
		if (IsEndless())
		{
			EnsureEndlessRound(Wave + 1);
		}
		// T6: the first time ever a wave brings a new plushie (or the pincushion), meet it first
		if (!bAttract && Save)
		{
			TArray<FString> Newcomers;
			EnsureImpostorRoll(Wave + 1);
			FindNewcomers(Wave + 1, Newcomers);
			IntroQueue = Newcomers;
			if (ShowNextIntro())
			{
				Mode = EKPMode::Intro;
				return;
			}
		}
		++Wave;
		if (IsEndless())
		{
			SaveEndlessRound(); // T6d: Continue starts this round again
		}
		const FKPWave& Data = CurrentWave();
		// T6c: this week's dark waves (after Thursday), announced as they start
		const bool bDarkNight = CurrentNight().Rule.Equals(TEXT("lightsout"), ESearchCase::IgnoreCase);
		const bool bDark = bDarkNight || IsDarkWave(NightIndex, Wave);
		SetLightsOut(bDark);
		if (bDark && !bDarkNight)
		{
			AddScreenText(FVector2D(0.5, 0.28), Text(TEXT("lightsOut"), TEXT("Lights out!")), 1.8f, 28.f, KPVisuals::Hex(TEXT("#FF4F8F")));
		}
		// The spawn list: picks from the mix, with any extras (a teaser) mixed into its second half
		SpawnList.Init(-1, FMath::Max(0, FMath::RoundToInt(Data.Count * WaveCountMul) + ExtraPerWave));
		for (const TPair<FString, int32>& Extra : Data.Extra)
		{
			const int32 TypeIndex = FindType(Extra.Key);
			for (int32 N = 0; TypeIndex != INDEX_NONE && N < Extra.Value; ++N)
			{
				SpawnList.Insert(TypeIndex, FMath::RandRange(SpawnList.Num() / 2, SpawnList.Num()));
			}
		}
		Queue = SpawnList.Num();
		SpawnTimer = 0.f;
		bBreakDone = false;
		bWaveClearSounded = bAttract; // (the title's mat makes no wave sounds)
		if (!Data.Boss.IsEmpty())
		{
			SpawnBoss(Data.Boss, Data.BossCount, Data.BossHits);
		}
		MagnetLeft = MagnetLevel;
		EnsureImpostorRoll(Wave);
		ImpostorTimer = bImpostorRoll ? FMath::FRandRange(2.f, 5.f) : -1.f; // T7d: this wave's impostor, a little way in
		BreakTimer = Tuning.PickDelay;
		AddScreenText(FVector2D(0.5, 0.36), IsEndless() ? FString::Printf(TEXT("Round %d"), Wave) : FString::Printf(TEXT("Wave %d of %d"), Wave, GetNumWaves()), 1.3f, 22.f, FLinearColor::White);
		// Some nights put a share of the pincushions in her way (placed later, once plushies are near)
		const int32 TotalPins = Data.Pins + ExtraPins;
		PathPinsLeft = FMath::Clamp(FMath::RoundToInt(TotalPins * CurrentNight().PinsInPath), 0, TotalPins);
		PathPinTimer = Tuning.PinPathFirst;
		for (int32 Index = 0; Index < TotalPins - PathPinsLeft; ++Index)
		{
			SpawnPin();
		}
	}
}

void AKPDirector::FadeOutPins()
{
	for (AKPPincushion* Pin : Pins)
	{
		if (IsValid(Pin))
		{
			Pin->FadeOut();
		}
	}
}

bool AKPDirector::SpawnPinInPath()
{
	// Between her and the nearest group of plushies: find the nearest one on the mat, the middle of
	// the plushies around it, and put the pincushion part way along the gap
	const float Hx = Tuning.ArenaHalfX;
	const float Hy = Tuning.ArenaHalfY;
	const AKPPlushie* Nearest = nullptr;
	float NearestDistance = TNumericLimits<float>::Max();
	for (const AKPPlushie* Plushie : Plushies)
	{
		if (!IsValid(Plushie) || Plushie->bDead || Plushie->bMarked)
		{
			continue;
		}
		const FVector Location = Plushie->GetActorLocation();
		if (FMath::Abs(Location.X) > Hx || FMath::Abs(Location.Y) > Hy)
		{
			continue;
		}
		const float Distance = FVector::Dist2D(Location, HeroPos);
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = Plushie;
		}
	}
	if (!Nearest)
	{
		return false;
	}
	FVector Group = FVector::ZeroVector;
	int32 InGroup = 0;
	for (const AKPPlushie* Plushie : Plushies)
	{
		if (IsValid(Plushie) && !Plushie->bDead && FVector::Dist2D(Plushie->GetActorLocation(), Nearest->GetActorLocation()) < 350.f)
		{
			Group += Plushie->GetActorLocation();
			++InGroup;
		}
	}
	Group /= FMath::Max(1, InGroup);
	FVector ToGroup = Group - HeroPos;
	ToGroup.Z = 0.f;
	const float Gap = ToGroup.Size();
	if (Gap < Tuning.PinMinDistance + 60.f)
	{
		return false; // too close to her already: a pincushion there would be unfair
	}
	const FVector Dir = ToGroup / Gap;
	const FVector Side(-Dir.Y, Dir.X, 0.f);
	const float LimitX = Hx - Tuning.PinEdge;
	const float LimitY = Hy - Tuning.PinEdge;
	for (int32 Try = 0; Try < 8; ++Try)
	{
		const float Along = FMath::Max(Tuning.PinMinDistance, Gap * FMath::FRandRange(0.4f, 0.65f));
		FVector Pos = HeroPos + Dir * Along + Side * FMath::FRandRange(-80.f, 80.f);
		Pos.X = FMath::Clamp(Pos.X, -LimitX, LimitX);
		Pos.Y = FMath::Clamp(Pos.Y, -LimitY, LimitY);
		Pos.Z = 0.f;
		if (FVector::Dist2D(Pos, HeroPos) < Tuning.PinMinDistance)
		{
			continue;
		}
		const bool bCrowded = Pins.ContainsByPredicate([&](const AKPPincushion* Other)
		{
			return IsValid(Other) && FVector::Dist2D(Other->GetActorLocation(), Pos) < Other->Radius + Tuning.PinRadius + 40.f;
		});
		if (bCrowded)
		{
			continue;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AKPPincushion* Pin = GetWorld()->SpawnActor<AKPPincushion>(AKPPincushion::StaticClass(), Pos, FRotator(0.f, FMath::FRand() * 360.f, 0.f), Params);
		if (Pin)
		{
			Pin->Init(Tuning.PinRadius, 1.0e6f, !bAttract && FMath::FRand() < Tuning.GoldenPinChance, RollBlinkPin()); // stays until the wave ends (T7b: a rare golden one)
			Pin->BlinkTimer = Tuning.BlinkEvery + FMath::FRandRange(0.f, 1.5f);
			Pins.Add(Pin);
			UE_LOG(LogKP, Log, TEXT("Pincushion in her way at (%.0f, %.0f)"), Pos.X, Pos.Y);
			return true;
		}
	}
	return false;
}

void AKPDirector::LayEggs(float GameDt)
{
	// A mother hen lays a chick every LayEvery seconds. Each chick hops off in a random direction (onto the
	// mat), then comes at her from there, so they don't trail in a line.
	using namespace KPDirectorConst;
	const float LimitX = Tuning.ArenaHalfX - Tuning.EdgeMargin - 30.f;
	const float LimitY = Tuning.ArenaHalfY - Tuning.EdgeMargin - 30.f;
	struct FBirth { int32 Type; FVector From; FVector To; };
	TArray<FBirth, TInlineAllocator<8>> Births;
	for (AKPPlushie* Hen : Plushies)
	{
		if (!IsValid(Hen) || Hen->bDead || Hen->bMarked || Hen->LayTimer < 0.f || Hen->PopIn > 0.f || !Types.IsValidIndex(Hen->TypeIndex))
		{
			continue;
		}
		// She lays from the moment she spawns, even off the mat (the chicks hop onto it)
		const FVector HenPos = Hen->GetActorLocation();
		Hen->LayTimer -= GameDt;
		if (Hen->LayTimer > 0.f)
		{
			continue;
		}
		const FKPPlushieType& Type = Types[Hen->TypeIndex];
		const int32 ChickType = FindType(Type.Lays);
		if (ChickType == INDEX_NONE || FVector::Dist2D(HenPos, HeroPos) < ChickSafeDistance)
		{
			Hen->LayTimer = 0.25f; // too close to her for a fair chick: try again in a moment
			continue;
		}
		Hen->LayTimer = Type.LayEvery;
		// A landing spot 200-350 away, on the mat and not close to her (the farthest try if none is)
		FVector Landing = HenPos;
		float BestDistance = -1.f;
		for (int32 Try = 0; Try < 10; ++Try)
		{
			const float Angle = FMath::FRand() * UE_TWO_PI;
			const float Hop = FMath::FRandRange(200.f, 350.f);
			FVector Spot = HenPos + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Hop;
			Spot.X = FMath::Clamp(Spot.X, -LimitX, LimitX);
			Spot.Y = FMath::Clamp(Spot.Y, -LimitY, LimitY);
			Spot.Z = 0.f;
			const float FromHer = FVector::Dist2D(Spot, HeroPos);
			if (FromHer >= ChickSafeDistance)
			{
				Landing = Spot;
				break;
			}
			if (FromHer > BestDistance)
			{
				BestDistance = FromHer;
				Landing = Spot;
			}
		}
		Births.Add({ ChickType, FVector(HenPos.X, HenPos.Y, 0.f), Landing });
	}
	for (const FBirth& Birth : Births)
	{
		const FKPPlushieType& Chick = Types[Birth.Type];
		const FLinearColor Color = Chick.Colors.Num() > 0 ? KPVisuals::Hex(*Chick.Colors[FMath::RandRange(0, Chick.Colors.Num() - 1)]) : FLinearColor::White;
		if (AKPPlushie* Baby = SpawnPlushieAt(Birth.Type, Birth.From, FMath::FRandRange(Chick.RadiusMin, Chick.RadiusMax), Color, WaveSpeed() * Chick.SpeedMul))
		{
			Baby->ScatterFrom = Birth.From;
			Baby->ScatterTo = Birth.To;
			Baby->ScatterT = 0.f;
			Baby->UpdatePose(FVector(-1.f, 0.f, 0.f), 0.f, false, 0.f, 0.01f);
			Burst(Baby->GetBodyCenter(), 5, FVector::ZeroVector, false);
		}
	}
}

int32 AKPDirector::FindType(const FString& Name) const
{
	return Types.IndexOfByPredicate([&Name](const FKPPlushieType& Type) { return Type.Name.Equals(Name, ESearchCase::IgnoreCase); });
}

int32 AKPDirector::PickType() const
{
	// Golden: a rare roll on any spawn, on the nights that allow it
	const FKPNight& Night = CurrentNight();
	const int32 Golden = FindType(TEXT("golden"));
	if (Golden != INDEX_NONE && FMath::FRand() < Night.GoldenChance)
	{
		return Golden;
	}
	// T7d: now and then a variant (two types in one), once both of its types have turned up this week
	const bool bVariantsOn = !bAttract && (IsEndless() ? Wave >= Tuning.VariantFromRound : NightIndex + 1 >= Tuning.VariantFromNight);
	if (bVariantsOn && FMath::FRand() < Tuning.VariantChance)
	{
		const int32 Variant = PickVariant();
		if (Variant != INDEX_NONE)
		{
			return Variant;
		}
	}
	// The wave's mix, or the night's
	const TMap<FString, float>& Mix = CurrentWave().Mix.Num() > 0 ? CurrentWave().Mix : Night.Mix;
	float Total = 0.f;
	for (const TPair<FString, float>& Entry : Mix)
	{
		Total += FindType(Entry.Key) != INDEX_NONE ? FMath::Max(0.f, Entry.Value) : 0.f;
	}
	float Roll = FMath::FRand() * Total;
	for (const TPair<FString, float>& Entry : Mix)
	{
		const int32 Index = FindType(Entry.Key);
		if (Index == INDEX_NONE || Entry.Value <= 0.f)
		{
			continue;
		}
		Roll -= Entry.Value;
		if (Roll <= 0.f)
		{
			return Index;
		}
	}
	const int32 Teddy = FindType(TEXT("teddy"));
	return Teddy != INDEX_NONE ? Teddy : 0;
}

bool AKPDirector::TypeSeenThisWeek(const FString& Name) const
{
	const bool bGolden = Name.Equals(TEXT("golden"), ESearchCase::IgnoreCase);
	auto InMix = [&Name](const TMap<FString, float>& Mix)
	{
		const float* Weight = Mix.Find(Name);
		return Weight && *Weight > 0.f;
	};
	if (IsEndless())
	{
		return InMix(CurrentWave().Mix) || (bGolden && Endless.GoldenChance > 0.f);
	}
	for (int32 Night = 0; Night <= NightIndex && Night < Nights.Num(); ++Night)
	{
		const FKPNight& Each = Nights[Night];
		if (InMix(Each.Mix) || (bGolden && Each.GoldenChance > 0.f))
		{
			return true;
		}
		for (const FKPWave& EachWave : Each.Waves)
		{
			const int32* Extra = EachWave.Extra.Find(Name);
			if (InMix(EachWave.Mix) || (Extra && *Extra > 0))
			{
				return true;
			}
		}
	}
	return false;
}

int32 AKPDirector::PickVariant() const
{
	TArray<int32> Ready;
	for (int32 Index = 0; Index < Types.Num(); ++Index)
	{
		const FKPPlushieType& Type = Types[Index];
		if (!Type.Variant || Type.Needs.Num() == 0)
		{
			continue;
		}
		bool bAll = true;
		for (const FString& Need : Type.Needs)
		{
			bAll = bAll && TypeSeenThisWeek(Need);
		}
		if (bAll)
		{
			Ready.Add(Index);
		}
	}
	return Ready.Num() > 0 ? Ready[FMath::RandRange(0, Ready.Num() - 1)] : INDEX_NONE;
}

float AKPDirector::WaveSpeed() const
{
	return (CurrentWave().Speed + FMath::FRand() * Tuning.PlushieSpeedRandom) * SpeedMul;
}

bool AKPDirector::SpawnByName(const FString& TypeName, int32 Count)
{
	const int32 Index = FindType(TypeName);
	if (Index == INDEX_NONE)
	{
		if (TypeName.Equals(TEXT("pin"), ESearchCase::IgnoreCase) || TypeName.Equals(TEXT("pincushion"), ESearchCase::IgnoreCase))
		{
			for (int32 N = 0; N < Count; ++N)
			{
				SpawnPin();
			}
			return true;
		}
		return false;
	}
	for (int32 N = 0; N < Count; ++N)
	{
		SpawnPlushie(Index);
	}
	return true;
}

void AKPDirector::SpawnPin()
{
	UWorld* World = GetWorld();
	const float LimitX = Tuning.ArenaHalfX - Tuning.PinEdge;
	const float LimitY = Tuning.ArenaHalfY - Tuning.PinEdge;
	for (int32 Try = 0; Try < 20; ++Try)
	{
		const FVector Pos(FMath::FRandRange(-LimitX, LimitX), FMath::FRandRange(-LimitY, LimitY), 0.f);
		if (FVector::Dist2D(Pos, HeroPos) < Tuning.PinMinDistance)
		{
			continue;
		}
		const bool bCrowded = Pins.ContainsByPredicate([&](const AKPPincushion* Other)
		{
			return IsValid(Other) && FVector::Dist2D(Other->GetActorLocation(), Pos) < Other->Radius + Tuning.PinRadius + 40.f;
		});
		if (bCrowded)
		{
			continue;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AKPPincushion* Pin = World->SpawnActor<AKPPincushion>(AKPPincushion::StaticClass(), Pos, FRotator(0.f, FMath::FRand() * 360.f, 0.f), Params);
		if (Pin)
		{
			Pin->Init(Tuning.PinRadius, 1.0e6f, !bAttract && FMath::FRand() < Tuning.GoldenPinChance, RollBlinkPin()); // stays until the wave ends (FadeOutPins); T7b: a rare golden one
			Pin->BlinkTimer = Tuning.BlinkEvery + FMath::FRandRange(0.f, 1.5f); // (T7c: blinkers out of step)
			Pins.Add(Pin);
		}
		return;
	}
}

void AKPDirector::EnsureImpostorRoll(int32 WaveNumber)
{
	if (ImpostorRollWave == WaveNumber)
	{
		return;
	}
	ImpostorRollWave = WaveNumber;
	bImpostorRoll = false;
	const FKPNight& Night = CurrentNight();
	if (bAttract || FindType(TEXT("impostor")) == INDEX_NONE || !Night.Waves.IsValidIndex(WaveNumber - 1))
	{
		return;
	}
	// Never in the boss wave or a dark wave
	const FKPWave& Data = Night.Waves[WaveNumber - 1];
	const bool bDark = Night.Rule.Equals(TEXT("lightsout"), ESearchCase::IgnoreCase) || IsDarkWave(NightIndex, WaveNumber);
	if (!Data.Boss.IsEmpty() || bDark)
	{
		return;
	}
	const bool bOn = IsEndless() ? WaveNumber >= Tuning.ImpostorFromRound : NightIndex + 1 >= Tuning.ImpostorFromNight;
	bImpostorRoll = bOn && FMath::FRand() < (IsEndless() ? Tuning.ImpostorRoundChance : Tuning.ImpostorChance);
}

bool AKPDirector::SpawnImpostor()
{
	const bool bGold = FMath::FRand() < Tuning.GoldImpostorChance;
	int32 TypeIndex = FindType(bGold ? TEXT("goldimpostor") : TEXT("impostor"));
	TypeIndex = TypeIndex != INDEX_NONE ? TypeIndex : FindType(TEXT("impostor"));
	if (TypeIndex == INDEX_NONE)
	{
		return false;
	}
	const FKPPlushieType& Type = Types[TypeIndex];
	const float Radius = FMath::FRandRange(Type.RadiusMin, Type.RadiusMax);
	// Well inside the mat (at least ImpostorEdge from every edge), away from her, the pincushions and the plushies
	const float LimitX = FMath::Max(0.f, Tuning.ArenaHalfX - Tuning.ImpostorEdge);
	const float LimitY = FMath::Max(0.f, Tuning.ArenaHalfY - Tuning.ImpostorEdge);
	for (int32 Try = 0; Try < 30; ++Try)
	{
		const FVector Pos(FMath::FRandRange(-LimitX, LimitX), FMath::FRandRange(-LimitY, LimitY), 0.f);
		if (FVector::Dist2D(Pos, HeroPos) < Tuning.PinMinDistance + 60.f)
		{
			continue;
		}
		const bool bCrowded = Pins.ContainsByPredicate([&](const AKPPincushion* Other) { return IsValid(Other) && FVector::Dist2D(Other->GetActorLocation(), Pos) < Other->Radius + Radius + 60.f; })
			|| Plushies.ContainsByPredicate([&](const AKPPlushie* Other) { return IsValid(Other) && FVector::Dist2D(Other->GetActorLocation(), Pos) < Other->Radius + Radius + 30.f; });
		if (bCrowded)
		{
			continue;
		}
		const FLinearColor Color = KPVisuals::Hex(Type.Colors.Num() > 0 ? *Type.Colors[0] : TEXT("#E0344F"));
		AKPPlushie* Impostor = SpawnPlushieAt(TypeIndex, Pos, Radius, Color, Tuning.ImpostorSpeed);
		if (!Impostor)
		{
			return false;
		}
		// Its way out: the edge nearest to where it sat down
		const float ToTop = Tuning.ArenaHalfX - Pos.X;
		const float ToBottom = Tuning.ArenaHalfX + Pos.X;
		const float ToRight = Tuning.ArenaHalfY - Pos.Y;
		const float ToLeft = Tuning.ArenaHalfY + Pos.Y;
		const float Nearest = FMath::Min(FMath::Min(ToTop, ToBottom), FMath::Min(ToRight, ToLeft));
		const float Beyond = Radius * 2.f;
		Impostor->ImpostorExit = Nearest == ToTop ? FVector(Tuning.ArenaHalfX + Beyond, Pos.Y, 0.f)
			: Nearest == ToBottom ? FVector(-Tuning.ArenaHalfX - Beyond, Pos.Y, 0.f)
			: Nearest == ToRight ? FVector(Pos.X, Tuning.ArenaHalfY + Beyond, 0.f)
			: FVector(Pos.X, -Tuning.ArenaHalfY - Beyond, 0.f);
		Impostor->ImpostorAge = 0.f;
		Impostor->PopIn = 0.25f;
		Impostor->UpdatePose(FVector(-1.f, 0.f, 0.f), 0.f, false, 0.f, 0.01f);
		Burst(Impostor->GetBodyCenter(), 8, FVector::ZeroVector, false);
		UE_LOG(LogKP, Log, TEXT("Impostor%s at (%.0f, %.0f)"), bGold ? TEXT(" (gold)") : TEXT(""), Pos.X, Pos.Y);
		return true;
	}
	return false;
}

bool AKPDirector::CanBlinkPinsNow(int32 WaveNumber) const
{
	if (bAttract)
	{
		return false;
	}
	// T7e: never in a dark wave (Lights out, this week's dark waves, Extended nightmare): those spots get normal ones
	if (CurrentNight().Rule.Equals(TEXT("lightsout"), ESearchCase::IgnoreCase) || IsDarkWave(NightIndex, WaveNumber))
	{
		return false;
	}
	return IsEndless() ? WaveNumber >= Tuning.BlinkFromRound : NightIndex + 1 >= Tuning.BlinkFromNight;
}

bool AKPDirector::RollBlinkPin() const
{
	return CanBlinkPinsNow(Wave) && FMath::FRand() < Tuning.BlinkChance;
}

bool AKPDirector::FindPinSpot(FVector& Out, const AKPPincushion* Ignore) const
{
	const float LimitX = Tuning.ArenaHalfX - Tuning.PinEdge;
	const float LimitY = Tuning.ArenaHalfY - Tuning.PinEdge;
	for (int32 Try = 0; Try < 30; ++Try)
	{
		const FVector Pos(FMath::FRandRange(-LimitX, LimitX), FMath::FRandRange(-LimitY, LimitY), 0.f);
		if (FVector::Dist2D(Pos, HeroPos) < Tuning.PinMinDistance)
		{
			continue;
		}
		const bool bCrowded = Pins.ContainsByPredicate([&](const AKPPincushion* Other)
		{
			return IsValid(Other) && Other != Ignore && (FVector::Dist2D(Other->GetActorLocation(), Pos) < Other->Radius + Tuning.PinRadius + 40.f
				|| (Other->bBlinkAiming && FVector::Dist2D(Other->BlinkTo, Pos) < Other->Radius + Tuning.PinRadius + 40.f));
		});
		if (!bCrowded)
		{
			Out = Pos;
			return true;
		}
	}
	return false;
}

void AKPDirector::UpdatePins(float GameDt)
{
	for (int32 Index = Pins.Num() - 1; Index >= 0; --Index)
	{
		AKPPincushion* Pin = Pins[Index];
		if (IsValid(Pin) && Pin->bBlink && !Pin->IsFading())
		{
			// T7c: the blinking pincushion: a faint ring where it will be (UpdatePreview draws it), pulsing with
			// a low tick, then it's there
			Pin->BlinkTimer -= GameDt;
			const float Warn = FMath::Max(0.1f, Tuning.BlinkWarn);
			if (!Pin->bBlinkAiming && Pin->BlinkTimer <= Warn)
			{
				Pin->bBlinkAiming = FindPinSpot(Pin->BlinkTo, Pin);
				Pin->BlinkPulsesDone = 0;
				if (!Pin->bBlinkAiming)
				{
					Pin->BlinkTimer = Warn + 0.5f; // nowhere free: stays a little longer
				}
			}
			if (Pin->bBlinkAiming)
			{
				const int32 Pulses = FMath::Max(1, Tuning.BlinkPulses);
				if (Pin->BlinkPulsesDone < Pulses && Pin->BlinkTimer <= Warn * (1.f - static_cast<float>(Pin->BlinkPulsesDone) / Pulses))
				{
					++Pin->BlinkPulsesDone;
					PlaySound(TEXT("blink")); // (its minGap keeps it to one at a time)
				}
				if (Pin->BlinkTimer <= 0.f)
				{
					Pin->Reappear(Pin->BlinkTo);
					Pin->bBlinkAiming = false;
					Pin->BlinkTimer = Tuning.BlinkEvery;
				}
			}
		}
		if (IsValid(Pin) && Pin->Advance(GameDt))
		{
			continue;
		}
		if (IsValid(Pin))
		{
			Pin->Destroy();
		}
		Pins.RemoveAt(Index);
	}
}

bool AKPDirector::HasPendingSplits() const
{
	return CutPlushies.ContainsByPredicate([](const AKPPlushie* Plushie) { return IsValid(Plushie) && Plushie->SplitTimer >= 0.f; });
}

void AKPDirector::UpdateSplits(float GameDt)
{
	// A cut big bear: after a short beat its two halves get up as two small teddies (a pop, not a fade)
	const float LimitX = Tuning.ArenaHalfX - Tuning.EdgeMargin;
	const float LimitY = Tuning.ArenaHalfY - Tuning.EdgeMargin;
	for (AKPPlushie* Parent : CutPlushies)
	{
		if (!IsValid(Parent) || Parent->SplitTimer < 0.f)
		{
			continue;
		}
		Parent->SplitTimer -= GameDt;
		if (Parent->SplitTimer > 0.f)
		{
			continue;
		}
		Parent->SplitTimer = -1.f;
		const FKPPlushieType* ParentType = Types.IsValidIndex(Parent->TypeIndex) ? &Types[Parent->TypeIndex] : nullptr;
		const int32 ChildType = ParentType ? FindType(ParentType->SplitInto) : INDEX_NONE;
		FVector HalfA;
		FVector HalfB;
		if (ChildType == INDEX_NONE || !Parent->GetHalfLocations(HalfA, HalfB))
		{
			continue;
		}
		const FKPPlushieType& Child = Types[ChildType];
		const FVector Halves[2] = { HalfA, HalfB };
		for (const FVector& Half : Halves)
		{
			const FVector Pos(FMath::Clamp(Half.X, -LimitX, LimitX), FMath::Clamp(Half.Y, -LimitY, LimitY), 0.f);
			const float Radius = FMath::FRandRange(Child.RadiusMin, Child.RadiusMax);
			if (AKPPlushie* Cub = SpawnPlushieAt(ChildType, Pos, Radius, Parent->Color, WaveSpeed() * Child.SpeedMul))
			{
				Cub->PopIn = KPDirectorConst::PopInTime;
				Cub->UpdatePose(FVector(-1.f, 0.f, 0.f), 0.f, false, 0.f, 0.01f);
				Burst(Cub->GetBodyCenter(), 10, FVector::ZeroVector, false);
			}
		}
		Parent->PieceLife = 0.f; // the halves are gone: they got up
		AddWorldText(Parent->GetActorLocation() + FVector(0.f, 0.f, 120.f), TEXT("Up again!"), FString(), 0.9f, 13.f, FLinearColor::White);
	}
}

AKPPlushie* AKPDirector::SpawnPlushie(int32 TypeIndex)
{
	if (!Types.IsValidIndex(TypeIndex))
	{
		return nullptr;
	}
	const FKPPlushieType& Type = Types[TypeIndex];
	const float Radius = FMath::FRandRange(Type.RadiusMin, Type.RadiusMax);
	const float Hx = Tuning.ArenaHalfX;
	const float Hy = Tuning.ArenaHalfY;
	const float Pad = Tuning.SpawnPad + FMath::Max(0.f, Radius - Tuning.PlushieRadiusMax);

	// Just outside a random edge of the mat
	FVector Pos(FMath::FRandRange(-Hx, Hx), FMath::FRandRange(-Hy, Hy), 0.f);
	switch (FMath::RandRange(0, 3))
	{
	case 0: Pos.X = Hx + Pad; break;
	case 1: Pos.Y = Hy + Pad; break;
	case 2: Pos.X = -Hx - Pad; break;
	default: Pos.Y = -Hy - Pad; break;
	}

	const FLinearColor Color = Type.Colors.Num() > 0
		? KPVisuals::Hex(*Type.Colors[FMath::RandRange(0, Type.Colors.Num() - 1)])
		: KPVisuals::Hex(KPDirectorConst::Pastels[FMath::RandRange(0, KPDirectorConst::NumPastels - 1)]);
	return SpawnPlushieAt(TypeIndex, Pos, Radius, Color, WaveSpeed() * Type.SpeedMul);
}

AKPPlushie* AKPDirector::SpawnPlushieAt(int32 TypeIndex, const FVector& Pos, float Radius, const FLinearColor& Color, float Speed)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AKPPlushie* Plushie = GetWorld()->SpawnActor<AKPPlushie>(AKPPlushie::StaticClass(), Pos, FRotator(0.f, 180.f, 0.f), Params);
	if (!Plushie)
	{
		return nullptr;
	}
	Plushie->Init(Types[TypeIndex], TypeIndex, Radius, Color, Speed);
	if (!bAttract)
	{
		// T7e: counted for the wave's log line (the golden odds check)
		++WaveSpawned;
		const FString& TypeName = Types[TypeIndex].Name;
		WaveGolden += TypeName.StartsWith(TEXT("gold")) ? 1 : 0;
		WaveGoldenRolled += TypeName.Equals(TEXT("golden")) ? 1 : 0;
	}
	// T7d: the first time a variant ever shows up: a small toast at the top (no full introduction)
	if (Types[TypeIndex].Variant && !bAttract && Save && !Save->MetTypes.Contains(TEXT("newvariant:") + Types[TypeIndex].Name))
	{
		if (!bAutoCut)
		{
			Save->MetTypes.Add(TEXT("newvariant:") + Types[TypeIndex].Name); // (test runs don't use up the toast)
			SaveProgress();
		}
		const FString Label = Types[TypeIndex].Label.IsEmpty() ? Types[TypeIndex].Name : Types[TypeIndex].Label;
		AddScreenText(FVector2D(0.5, 0.1), Text(TEXT("newVariant"), TEXT("New: {name}")).Replace(TEXT("{name}"), *Label), 2.4f, 18.f, KPVisuals::Hex(TEXT("#FFE14D")));
		UE_LOG(LogKP, Log, TEXT("New variant: %s"), *Label);
	}
	if (Types[TypeIndex].Look == TEXT("chick") && !bAttract)
	{
		PlaySound(TEXT("peep")); // T7b (minGap keeps a flock to a few peeps)
	}
	if (!Types[TypeIndex].Lays.IsEmpty())
	{
		Plushie->CircleDir = FMath::RandBool() ? 1.f : -1.f; // T7c
		Plushie->LayTimer = Types[TypeIndex].LayEvery; // she arrives with her first brood (below), then keeps laying (LayEggs)
	}
	Plushies.Add(Plushie);

	// A mother hen arrives with chicks that burst out and rush straight at her (no scatter hop first)
	const int32 ChickType = Types[TypeIndex].Lays.IsEmpty() ? INDEX_NONE : FindType(Types[TypeIndex].Lays);
	for (int32 N = 0; ChickType != INDEX_NONE && N < Types[TypeIndex].ArrivesWith; ++N)
	{
		const FKPPlushieType& Chick = Types[ChickType];
		const float Angle = UE_TWO_PI * N / FMath::Max(1, Types[TypeIndex].ArrivesWith) + FMath::FRand();
		const FVector At = Pos + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (Radius + Chick.RadiusMax);
		if (FVector::Dist2D(At, HeroPos) < KPDirectorConst::ChickSafeDistance)
		{
			continue;
		}
		const FLinearColor ChickColor = Chick.Colors.Num() > 0 ? KPVisuals::Hex(*Chick.Colors[FMath::RandRange(0, Chick.Colors.Num() - 1)]) : FLinearColor::White;
		if (AKPPlushie* Baby = SpawnPlushieAt(ChickType, FVector(At.X, At.Y, 0.f), FMath::FRandRange(Chick.RadiusMin, Chick.RadiusMax), ChickColor, WaveSpeed() * Chick.SpeedMul))
		{
			Baby->PopIn = 0.15f;
			Baby->UpdatePose(FVector(-1.f, 0.f, 0.f), 0.f, false, 0.f, 0.01f);
		}
	}
	return Plushie;
}

void AKPDirector::UpdatePlushies(float GameDt, float RealDt)
{
	const int32 Num = Plushies.Num();
	TArray<FVector, TInlineAllocator<64>> Positions;
	TArray<float, TInlineAllocator<64>> Lifts;
	TArray<float, TInlineAllocator<64>> Steps;   // how far each walked this frame (for the slide around pincushions)
	Positions.SetNum(Num);
	Lifts.SetNumZeroed(Num);
	Steps.SetNumZeroed(Num);
	for (int32 Index = 0; Index < Num; ++Index)
	{
		Positions[Index] = IsValid(Plushies[Index]) ? Plushies[Index]->GetActorLocation() : FVector::ZeroVector;
	}

	// Walk straight at her; a touch outside the dash and the beat costs a heart
	const bool bCanBeHugged = Mode != EKPMode::Dash && Mode != EKPMode::Snip;
	for (int32 Index = 0; Index < Num; ++Index)
	{
		AKPPlushie* Plushie = Plushies[Index];
		if (!IsValid(Plushie) || Plushie->bMarked || Plushie->bDead)
		{
			continue;
		}
		const FKPPlushieType* Type = Types.IsValidIndex(Plushie->TypeIndex) ? &Types[Plushie->TypeIndex] : nullptr;
		// The patchwork ignores slow motion: it lives on real time
		const bool bRealTime = Type && Type->IgnoreSlowMo;
		const float Dt = bRealTime ? RealDt : GameDt;
		Plushie->Wobble += Dt * (Plushie->bBoss ? 3.5f : bRealTime ? 15.f : 9.f);
		if (Plushie->KnockLeft > 0.f)
		{
			// The boss, knocked back after touching her: slides away, can't hug her meanwhile
			const float Knock = FMath::Min(Plushie->KnockLeft, 1100.f * Dt);
			const FVector Before = Positions[Index];
			Positions[Index] += Plushie->KnockDir * Knock;
			Plushie->KnockLeft -= Knock;
			// T6c: it stays wholly on the mat (and clear of the HUD's night name at the top, +X); one still
			// walking in from outside isn't pulled in
			const float TopX = FMath::Max(Tuning.ArenaHalfX - Plushie->Radius - 120.f, Before.X);
			const float BottomX = FMath::Min(-(Tuning.ArenaHalfX - Plushie->Radius), Before.X);
			const float LimitY = FMath::Max(Tuning.ArenaHalfY - Plushie->Radius, FMath::Abs(Before.Y));
			Positions[Index].X = FMath::Clamp(Positions[Index].X, BottomX, TopX);
			Positions[Index].Y = FMath::Clamp(Positions[Index].Y, -LimitY, LimitY);
			continue;
		}
		if (Plushie->bPatchwork && FMath::FRand() < RealDt * 18.f)
		{
			// A jittery, twitchy walk
			Plushie->PoseJitter = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(0.f, 1.2f)) * (Plushie->Radius * 0.07f);
		}
		if (Plushie->PopIn > 0.f)
		{
			// Just got up (a split bear's cub): stays put and can't hug her yet
			Plushie->PopIn -= Dt;
			continue;
		}
		if (Plushie->ScatterT >= 0.f)
		{
			// A chick's hop away from the hen: in the air, it can't hug her
			Plushie->ScatterT += Dt / KPDirectorConst::ChickHopTime;
			const float U = FMath::Min(Plushie->ScatterT, 1.f);
			Positions[Index] = FMath::Lerp(Plushie->ScatterFrom, Plushie->ScatterTo, U);
			Lifts[Index] = FMath::Sin(U * UE_PI) * KPDirectorConst::ChickHopHeight;
			if (Plushie->ScatterT >= 1.f)
			{
				Plushie->ScatterT = -1.f;
			}
			continue;
		}
		if (Plushie->bImpostor)
		{
			// T7d: the impostor: sits still and wiggles (the tell), then walks for the edge that was nearest,
			// stepping round her; it never hugs. Off the mat it has escaped: a heart.
			Plushie->ImpostorAge += Dt;
			if (Plushie->ImpostorAge < Tuning.ImpostorSit)
			{
				Plushie->PoseJitter = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f) * (Plushie->Radius * 0.06f);
				continue;
			}
			Plushie->PoseJitter = FVector::ZeroVector;
			FVector Dir = Plushie->ImpostorExit - Positions[Index];
			Dir.Z = 0.f;
			Dir = Dir.GetSafeNormal();
			FVector Away = Positions[Index] - HeroPos;
			Away.Z = 0.f;
			const float Clear = Tuning.HeroRadius + Plushie->Radius + 90.f;
			const float Near = Away.Size();
			if (Near < Clear && Near > 0.01f)
			{
				Dir = (Dir + Away / Near * (1.f - Near / Clear) * 2.f).GetSafeNormal();
			}
			const float Walk = Plushie->Speed * Dt;
			Positions[Index] += Dir * Walk;
			Steps[Index] = Walk;
			const float Out = Plushie->Radius * 0.6f;
			if (FMath::Abs(Positions[Index].X) > Tuning.ArenaHalfX + Out || FMath::Abs(Positions[Index].Y) > Tuning.ArenaHalfY + Out)
			{
				Plushie->bDead = true;
				Burst(Plushie->GetBodyCenter(), 8, FVector::ZeroVector, false);
				AddWorldText(Positions[Index] + FVector(0.f, 0.f, 140.f), Text(TEXT("escaped"), TEXT("Escaped!")), FString(), 1.2f, 15.f, KPVisuals::Hex(TEXT("#FF6E82")));
				UE_LOG(LogKP, Log, TEXT("Impostor escaped"));
				Hurt(FString(), TEXT("impostor"));
			}
			continue;
		}
		FVector ToHero = HeroPos - Positions[Index];
		ToHero.Z = 0.f;
		const float Distance = FMath::Max(ToHero.Size(), 0.001f);
		float Step = Plushie->Speed * Dt;
		if (Type && Type->Hop)
		{
			// Pause, hop, pause: the same ground on average, but all of it while in the air
			const float Cycle = FMath::Max(Type->HopPause + Type->HopTime, 0.05f);
			Plushie->HopClock = FMath::Fmod(Plushie->HopClock + Dt, Cycle);
			if (Plushie->HopClock < Type->HopPause)
			{
				Step = 0.f;
			}
			else
			{
				const float U = (Plushie->HopClock - Type->HopPause) / FMath::Max(Type->HopTime, 0.01f);
				Step *= Cycle / FMath::Max(Type->HopTime, 0.01f);
				Lifts[Index] = FMath::Sin(U * UE_PI) * Type->HopHeight;
			}
		}
		// T7c: (the hen) only once she's on the mat; until then she walks in like the others, so she never
		// circles off the mat out of reach when the heroine stands near an edge
		const float KeepLimitX = Tuning.ArenaHalfX - Plushie->Radius - 40.f;
		const float KeepLimitY = Tuning.ArenaHalfY - Plushie->Radius - 40.f;
		const bool bOnMat = Positions[Index].X >= -KeepLimitX && Positions[Index].X <= KeepLimitX - 80.f && FMath::Abs(Positions[Index].Y) <= KeepLimitY;
		if (Type && Type->KeepAway > 0.f && bOnMat && Distance < Type->KeepAway + Type->KeepAwayBand)
		{
			// T7c: the hen keeps her distance: circles her, backing away when she comes nearer, and stays on the mat
			const FVector Dir = ToHero / Distance;
			const FVector Tangent(-Dir.Y * Plushie->CircleDir, Dir.X * Plushie->CircleDir, 0.f);
			const float Back = Distance < Type->KeepAway - Type->KeepAwayBand ? Type->BackAwayMul
				: Distance < Type->KeepAway ? 1.f : 0.f;
			const FVector Before = Positions[Index];
			Positions[Index] += (Tangent * Type->CircleMul - Dir * Back) * (Plushie->Speed * Dt);
			const FVector Wanted = Positions[Index];
			Positions[Index].X = FMath::Clamp(Wanted.X, -KeepLimitX, KeepLimitX - 80.f);
			Positions[Index].Y = FMath::Clamp(Wanted.Y, -KeepLimitY, KeepLimitY);
			if (!Positions[Index].Equals(Wanted, 0.5f) && Back <= 0.f)
			{
				Plushie->CircleDir = -Plushie->CircleDir; // circling into the mat's edge: the other way round
			}
			Steps[Index] = (Positions[Index] - Before).Size2D();
		}
		else
		{
			Positions[Index] += ToHero / Distance * FMath::Min(Step, Distance);
			Steps[Index] = FMath::Min(Step, Distance);
		}

		if (bCanBeHugged && Invulnerable <= 0.f && Distance < Plushie->Radius + Tuning.HeroRadius - Tuning.TouchSlack && Plushie->bBoss)
		{
			// The boss doesn't pop: it costs a heart and gets knocked back
			Plushie->KnockDir = -ToHero / Distance;
			Plushie->KnockLeft = Type ? Type->Knockback : 300.f;
			Burst(HeroPos + FVector(0.f, 0.f, 80.f), 10, FVector::ZeroVector, false);
			UE_LOG(LogKP, Log, TEXT("Hug: the boss touched her"));
			Hurt(TEXT("Hugged"), TEXT("boss"));
		}
		else if (bCanBeHugged && Invulnerable <= 0.f && Distance < Plushie->Radius + Tuning.HeroRadius - Tuning.TouchSlack)
		{
			Plushie->bDead = true;
			Burst(Plushie->GetBodyCenter(), 8, FVector::ZeroVector, false);
			UE_LOG(LogKP, Log, TEXT("Hug: %s touched her"), Type ? *Type->Name : TEXT("?"));
			Hurt(TEXT("Hugged"));
		}
	}

	// Push apart so they don't stack
	for (int32 I = 0; I < Num; ++I)
	{
		const AKPPlushie* A = Plushies[I];
		if (!IsValid(A) || A->bMarked || A->bDead)
		{
			continue;
		}
		for (int32 J = I + 1; J < Num; ++J)
		{
			const AKPPlushie* B = Plushies[J];
			if (!IsValid(B) || B->bMarked || B->bDead)
			{
				continue;
			}
			FVector Delta = Positions[J] - Positions[I];
			Delta.Z = 0.f;
			const float Distance = Delta.Size();
			const float MinDistance = A->Radius + B->Radius;
			if (Distance > 0.01f && Distance < MinDistance)
			{
				// (the boss isn't pushed around: the other one gets out of its way)
				const FVector Push = Delta * ((MinDistance - Distance) / 2.f / Distance);
				Positions[I] -= B->bBoss ? Push * 2.f : A->bBoss ? FVector::ZeroVector : Push;
				Positions[J] += A->bBoss ? Push * 2.f : B->bBoss ? FVector::ZeroVector : Push;
			}
		}
	}

	// Step around pincushions instead of walking over them
	for (const AKPPincushion* Pin : Pins)
	{
		if (!IsValid(Pin))
		{
			continue;
		}
		const FVector PinPos = Pin->GetActorLocation();
		for (int32 Index = 0; Index < Num; ++Index)
		{
			const AKPPlushie* Plushie = Plushies[Index];
			if (!IsValid(Plushie) || Plushie->bMarked || Plushie->bDead)
			{
				continue;
			}
			FVector Delta = Positions[Index] - PinPos;
			Delta.Z = 0.f;
			const float Distance = Delta.Size();
			const float MinDistance = Plushie->Radius + Pin->Radius;
			if (Distance > 0.01f && Distance < MinDistance)
			{
				Positions[Index] += Delta * ((MinDistance - Distance) / Distance);
				// T6b: and slide along its edge (the side nearer her; dead centre: by index), so nobody stalls on one
				const FVector Normal = Delta / Distance;
				FVector Tangent(-Normal.Y, Normal.X, 0.f);
				FVector ToHero = HeroPos - Positions[Index];
				ToHero.Z = 0.f;
				const float Along = FVector::DotProduct(Tangent, ToHero.GetSafeNormal());
				if (FMath::Abs(Along) < 0.05f ? (Index % 2 == 1) : Along < 0.f)
				{
					Tangent = -Tangent;
				}
				Positions[Index] += Tangent * Steps[Index];
			}
		}
	}

	// Apply, and clear out the dead
	for (int32 Index = Num - 1; Index >= 0; --Index)
	{
		AKPPlushie* Plushie = Plushies[Index];
		if (!IsValid(Plushie))
		{
			Plushies.RemoveAt(Index);
			continue;
		}
		if (Plushie->bDead)
		{
			Plushie->Destroy();
			Plushies.RemoveAt(Index);
			continue;
		}
		if (!Plushie->bMarked)
		{
			Plushie->SetActorLocation(Positions[Index]);
			// Faces mostly toward the camera so the faces read, turned a little toward her
			FVector ToHero = HeroPos - Positions[Index];
			ToHero.Z = 0.f;
			const FVector Face = (FVector(-1.f, 0.f, 0.f) + ToHero.GetSafeNormal() * 0.6f).GetSafeNormal();
			const bool bHopper = Types.IsValidIndex(Plushie->TypeIndex) && Types[Plushie->TypeIndex].Hop;
			float Scale = Plushie->PopIn > 0.f ? KPMath::EaseOutBack(1.f - Plushie->PopIn / KPDirectorConst::PopInTime) : 1.f;
			if (Plushie->ScatterT >= 0.f)
			{
				Scale = KPMath::EaseOutBack(FMath::Clamp(Plushie->ScatterT / 0.4f, 0.f, 1.f)); // pops out of the hen as it hops
			}
			Plushie->UpdatePose(Face, Plushie->Wobble, !bHopper, Lifts[Index], Scale);
		}
	}
}

void AKPDirector::UpdateDash(float RealDt)
{
	DashTime += RealDt;
	float Move = RealDt * Tuning.DashSpeed;
	while (Move > 0.f && Mode == EKPMode::Dash && DashIndex < Path.Num() - 1)
	{
		const FVector A = Path[DashIndex];
		const FVector B = Path[DashIndex + 1];
		const float Length = FMath::Max(static_cast<float>(FVector::Dist2D(A, B)), 1e-4f);
		const float Step = FMath::Min(Move, Length - DashS);
		const FVector From = HeroPos;
		DashS += Step;
		Move -= Step;
		HeroPos = FMath::Lerp(A, B, DashS / Length);
		HeroYaw = FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X));
		Sweep(From, HeroPos, HeroYaw);

		// Afterimage dots along what she just covered
		const float Covered = FVector::Dist2D(From, HeroPos);
		const int32 Dots = FMath::Max(1, FMath::CeilToInt(Covered / KPDirectorConst::TrailSpacing));
		for (int32 Dot = 1; Dot <= Dots; ++Dot)
		{
			Trail.Add({ FMath::Lerp(From, HeroPos, static_cast<float>(Dot) / Dots), KPDirectorConst::TrailLife });
		}

		if (DashS >= Length - 1e-4f)
		{
			++DashIndex;
			DashS = 0.f;
		}
	}

	if (Mode == EKPMode::Dash && DashIndex >= Path.Num() - 1 && bExtraInch && !bReturning)
	{
		ApplyExtraInch();
	}
	if (Mode == EKPMode::Dash && DashIndex >= Path.Num() - 1 && bDoubleStitch && !bReturning && Path.Num() > 1)
	{
		// Double stitch: back along the same line, cutting again
		Algo::Reverse(Path);
		bReturning = true;
		DashIndex = 0;
		DashS = 0.f;
	}
	else if (Mode == EKPMode::Dash && DashIndex >= Path.Num() - 1)
	{
		// The beat: she stands (idle, T7d: no punch) with the shears open; they snap shut on the snip
		Mode = EKPMode::Snip;
		SnipTimer = Tuning.Beat;
		if (bBeatShots && bAutoCut && AutoShotsLeft > 0 && AutoShotTimer <= 0.f)
		{
			AutoShotTimer = 0.12f; // (-KPBeatShots: mid-beat, the shears open)
		}
		if (AKPHeroine* H = Hero.Get())
		{
			H->PlayIdle();
		}
	}
}

void AKPDirector::Sweep(const FVector& From, const FVector& To, float Yaw)
{
	// Pincushions: her blade through one costs a heart (once per pincushion)
	for (AKPPincushion* Pin : Pins)
	{
		if (IsValid(Pin) && !Pin->bHit && KPMath::SegmentDistance2D(Pin->GetActorLocation(), From, To) <= Pin->Radius + Tuning.BladePad)
		{
			Pin->bHit = true;
			PlaySound(TEXT("pin")); // T7: the blade through a pincushion
			if (Pin->bGolden)
			{
				// T7b: a golden one: a heart like any (unless Magnet saves it), but it's cut, scores big at the
				// snip and counts in this line's combo
				++GoldenPinsCut;
				GoldenPinSpots.Add(Pin->GetActorLocation());
				Pin->Jolt();
				Pin->FadeOut();
				Burst(Pin->GetActorLocation() + FVector(0.f, 0.f, Pin->Radius * 0.6f), 16, KPMath::CutNormal(Yaw), false);
				if (MagnetLeft > 0)
				{
					--MagnetLeft;
					AddWorldText(Pin->GetActorLocation() + FVector(0.f, 0.f, 140.f), TEXT("Magnet!"), FString(), 0.9f, 13.f, KPVisuals::Hex(TEXT("#FFD23F")));
				}
				else
				{
					Hurt(TEXT("Pins!"), TEXT("pincushion"));
				}
				continue;
			}
			if (MagnetLeft > 0)
			{
				// Magnet: this one is cut like a plushie instead, no heart lost (and no score)
				--MagnetLeft;
				Pin->Jolt();
				Pin->FadeOut();
				Burst(Pin->GetActorLocation() + FVector(0.f, 0.f, Pin->Radius * 0.6f), 12, KPMath::CutNormal(Yaw), false);
				AddWorldText(Pin->GetActorLocation() + FVector(0.f, 0.f, 140.f), TEXT("Magnet!"), FString(), 0.9f, 13.f, KPVisuals::Hex(TEXT("#FFD23F")));
				continue;
			}
			Pin->Jolt();
			Burst(Pin->GetActorLocation() + FVector(0.f, 0.f, Pin->Radius * 0.6f), 8, FVector::ZeroVector, false);
			Hurt(TEXT("Pins!"), TEXT("pincushion"));
		}
	}

	for (AKPPlushie* Plushie : Plushies)
	{
		if (!IsValid(Plushie) || Plushie->bDead)
		{
			continue;
		}
		// The cut goes where her path passed closest to the centre: the offset along the cut normal
		// (cross(cut direction, up)) from the centre to the closest point on this stretch of the path
		const FVector Center = Plushie->GetActorLocation();
		const FVector Closest = KPMath::ClosestPointOnSegment2D(Center, From, To);
		const float Distance = FVector::Dist2D(Center, Closest);
		if (Distance > Plushie->Radius + Tuning.BladePad || (Plushie->bMarked && Distance >= Plushie->CutDistance))
		{
			continue;
		}
		const FVector Normal = KPMath::CutNormal(Yaw);
		const float Offset = static_cast<float>(FVector::DotProduct(FVector(Closest.X - Center.X, Closest.Y - Center.Y, 0.f), Normal));
		if (!Plushie->bMarked)
		{
			Marked.Add(Plushie);
		}
		Plushie->SetMarked(Yaw, Offset);
		Plushie->CutDistance = Distance;
	}
}

void AKPDirector::DoSnip()
{
	const int32 RippedStart = PinTargets.Num();
	if (bSeamRipper)
	{
		SeamRip();
	}
	const int32 GoldenPins = GoldenPinsCut; // T7b: golden pincushions count in the combo
	const int32 Cut = Marked.Num() + PinTargets.Num() - RippedStart + GoldenPins; // everything this snip cuts
	// T7c: chicks (types with scoreMul 0) give no score and don't count in the combo
	auto IsNoCombo = [this](const AKPPlushie* Plushie)
	{
		return IsValid(Plushie) && !Plushie->bBoss && Types.IsValidIndex(Plushie->TypeIndex) && Types[Plushie->TypeIndex].ScoreMul <= 0.f;
	};
	int32 NoCombo = 0;
	for (const AKPPlushie* Plushie : Marked)
	{
		NoCombo += IsNoCombo(Plushie) ? 1 : 0;
	}
	for (int32 Index = RippedStart; Index < PinTargets.Num(); ++Index)
	{
		NoCombo += IsNoCombo(PinTargets[Index]) ? 1 : 0;
	}
	const int32 Count = FMath::Max(0, Cut - NoCombo); // the combo
	FVector Centroid = FVector::ZeroVector;
	float ScoreShares = 0.f;   // each plushie's ScoreMul (a golden one counts 5)
	int32 BossPoints = 0;      // the boss: a chunk (or the final split), counted in the combo
	for (AKPPlushie* Plushie : Marked)
	{
		if (!IsValid(Plushie))
		{
			continue;
		}
		if (Plushie->bBoss)
		{
			Centroid += Plushie->GetBodyCenter();
			BossPoints += CutBoss(Plushie);
			continue;
		}
		ScoreShares += Types.IsValidIndex(Plushie->TypeIndex) ? Types[Plushie->TypeIndex].ScoreMul : 1.f;
		Centroid += Plushie->GetBodyCenter();
		Plushie->bDead = true;
		SliceQueue.Add(Plushie); // out of the game from here on; cut in ProcessSliceQueue
	}
	// Seam ripper's targets count in this combo; they're cut when their pins land
	for (int32 Index = RippedStart; Index < PinTargets.Num(); ++Index)
	{
		if (IsValid(PinTargets[Index]))
		{
			ScoreShares += Types.IsValidIndex(PinTargets[Index]->TypeIndex) ? Types[PinTargets[Index]->TypeIndex].ScoreMul : 1.f;
			Centroid += PinTargets[Index]->GetBodyCenter();
		}
	}
	for (int32 Index = 0; Index < Plushies.Num(); ++Index)
	{
		AKPPlushie* Plushie = Plushies[Index];
		if (!IsValid(Plushie) || Plushie->bDead)
		{
			if (IsValid(Plushie) && !SliceQueue.Contains(Plushie) && !PinTargets.Contains(Plushie))
			{
				Plushie->Destroy();
			}
			Plushies.RemoveAt(Index--);
		}
	}
	ProcessSliceQueue();
	if (bAutoCut && !bAttract && AutoShotsLeft > 0 && AutoShotTimer <= 0.f && !bBeatShots)
	{
		AutoShotTimer = 0.45f;
	}

	BestCombo = FMath::Max(BestCombo, Count);
	if (TutorialStep == 1 || TutorialStep == 2)
	{
		// The first cut: the last drawing prompt, for a few seconds
		TutorialStep = 3;
		TutorialTimer = 4.f;
	}
	for (const FVector& Spot : GoldenPinSpots)
	{
		Centroid += Spot + FVector(0.f, 0.f, Tuning.PinRadius);
	}
	if (Cut > 0 && !bAttract)
	{
		Centroid /= Cut;
		// 100 x n x n when every plushie counts 1 (DESIGN.md); a golden one adds its extra share
		const float ModeMul = 1.f; // (T6c: modes have no multipliers)
		const int32 Gain = FMath::RoundToInt((Tuning.ScorePerSnip * Count * ScoreShares + BossPoints) * ScoreMul * ModeMul) + GoldenPins * Tuning.GoldenPinScore;
		Score += Gain;
		for (const FVector& Spot : GoldenPinSpots)
		{
			// T7b: the golden pincushion's own gold "+3500" (on top of the combo's score)
			AddWorldText(Spot + FVector(0.f, 0.f, 150.f), FString::Printf(TEXT("+%d"), Tuning.GoldenPinScore), FString(), 1.6f, 24.f, KPVisuals::Hex(TEXT("#FFC93C")));
			PlaySound(TEXT("goldenpin"));
			UE_LOG(LogKP, Log, TEXT("Golden pincushion cut: +%d on top, combo x%d, snip worth %d"), Tuning.GoldenPinScore, Count, Gain);
		}
		FString Name = KPDirectorConst::ComboNames[FMath::Min(5, Count)];
		if (Count > 1)
		{
			Name += FString::Printf(TEXT("  x%d"), Count);
		}
		const bool bKillingPart = Count >= 5;
		if (Count > 0) // T7c: a line through chicks only shows no combo text
		{
			AddWorldText(Centroid + FVector(0.f, 0.f, 70.f), Name, FString::Printf(TEXT("+%d"), Gain), 1.4f,
				bKillingPart ? 30.f : 15.f + Count * 2.f, bKillingPart ? KPVisuals::Hex(TEXT("#FFE14D")) : FLinearColor::White);
		}
		// T7b: a small shake, bigger from bigSnipCount; and a hitstop, longer for bigger cuts
		const int32 FeelCount = FMath::Max(1, Count);
		const float SnipShakeNow = FeelCount >= Tuning.BigSnipCount ? Tuning.BigSnipShake + (FeelCount - Tuning.BigSnipCount) * Tuning.SnipShakePer
			: Tuning.SnipShake + (FeelCount - 1) * Tuning.SnipShakePer;
		Shake = FMath::Max(Shake, FMath::Min(Tuning.SnipShakeMax, SnipShakeNow));
		HitStopTimer = FMath::Max(HitStopTimer, FMath::Min(Tuning.HitStopMax, Tuning.HitStop + (FeelCount - 1) * Tuning.HitStopPer));
		if (bKillingPart)
		{
			Flash = 0.18f;
			FlashColor = FLinearColor(FColor(255, 240, 180));
		}
	}

	// T7: one snip and a few squishes (boss hits too); a big combo plays killingpart (the boss's final split
	// plays its own, in StartFinale)
	PlaySnipSounds(Cut, Count >= Audio.KillingPartCombo && FinaleTimer <= 0.f && !bAttract);

	Marked.Reset();
	GoldenPinsCut = 0;
	GoldenPinSpots.Reset();
	Path.Reset();
	PathLength = 0.f;
	ShearsAfterSnip = 0.2f; // T7d: the closed shears stay in her hand a moment
	Mode = EKPMode::Play;
	TsTarget = FinaleTimer > 0.f ? Tuning.DashTimeScale : 1.f;
	Cooldown = Tuning.Cooldown;
}

void AKPDirector::ProcessSliceQueue()
{
	if (SliceQueue.Num() == 0)
	{
		return;
	}
	// A budget per frame, shared by the snip itself and the catch-up calls from UpdatePieces
	static uint64 BudgetFrame = 0;
	static int32 SlicedThisFrame = 0;
	if (BudgetFrame != GFrameCounter)
	{
		BudgetFrame = GFrameCounter;
		SlicedThisFrame = 0;
	}
	if (SlicedThisFrame >= KPDirectorConst::MaxSlicesPerFrame)
	{
		return;
	}
	const double SliceStart = FPlatformTime::Seconds();
	int32 Done = 0;
	while (SliceQueue.Num() > 0 && SlicedThisFrame < KPDirectorConst::MaxSlicesPerFrame)
	{
		AKPPlushie* Plushie = SliceQueue[0];
		SliceQueue.RemoveAt(0);
		if (!IsValid(Plushie))
		{
			continue;
		}
		const float YawRad = FMath::DegreesToRadians(Plushie->CutYaw);
		const FVector Normal(-FMath::Sin(YawRad), FMath::Cos(YawRad), 0.f);
		// Director's cut: blood, and splats that stay. Theatrical cut: white fluff, nothing left on the mat.
		const bool bBlood = !KPSettings::IsTheatrical();
		Burst(Plushie->GetBodyCenter(), bBlood ? 24 : 14, Normal, bBlood);
		Spray(Plushie->GetBodyCenter(), Normal, bBlood);
		if (bBlood)
		{
			StainSplat(Plushie->GetActorLocation(), Plushie->Radius, Normal);
		}
		if (Plushie->Slice(CapBase, Tuning))
		{
			Plushie->PieceLife = Tuning.HalfLife;
			CutPlushies.Add(Plushie); // lives on as two halves
			if (Types.IsValidIndex(Plushie->TypeIndex) && !Types[Plushie->TypeIndex].SplitInto.IsEmpty() && !bBossDown)
			{
				Plushie->SplitTimer = Types[Plushie->TypeIndex].SplitDelay; // a big bear: its halves get up (UpdateSplits)
			}
		}
		else
		{
			UE_LOG(LogKP, Warning, TEXT("Slicing %s failed; it pops instead"), *Plushie->GetName());
			Plushie->Destroy();
		}
		++Done;
		++SlicedThisFrame;
	}
	UE_LOG(LogKP, Log, TEXT("Snip: sliced %d in %.2f ms (%d waiting)"), Done, (FPlatformTime::Seconds() - SliceStart) * 1000.0, SliceQueue.Num());
}

void AKPDirector::Hurt(const FString& Why, const TCHAR* Cause)
{
	if (Invulnerable > 0.f || bGod || bAttract)
	{
		return;
	}
	--Hearts;
	LastHurtCause = Cause;
	PlaySound(TEXT("hurt")); // T7
	Invulnerable = Tuning.Invulnerable;
	Flash = 0.3f;
	FlashColor = FLinearColor(FColor(200, 20, 50));
	Shake = FMath::Max(Shake, 10.f);
	if (AKPHeroine* H = Hero.Get())
	{
		H->Hurt(); // T7f: her shake and red flash
	}
	if (!Why.IsEmpty())
	{
		AddWorldText(HeroPos + FVector(0.f, 0.f, 210.f), Why, FString(), 0.9f, 15.f, FLinearColor::White);
	}
	if (Hearts <= 0)
	{
		PlaySound(TEXT("gameover")); // T7b (the night music fades out under it)
		Mode = EKPMode::Over;
		TsTarget = Tuning.GameOverTimeScale;
		Path.Reset();
		PathLength = 0.f;
		RecordNightEnd(false); // the results screen, then the same night again
	}
}

// ---------------------------------------------------------------------------
// Juice

void AKPDirector::Burst(const FVector& At, int32 Count, const FVector& Normal, bool bBlood)
{
	const bool bHasNormal = !Normal.IsNearlyZero();
	for (int32 Index = 0; Index < Count && Particles.Num() < KPDirectorConst::MaxParticles; ++Index)
	{
		const float Angle = FMath::FRand() * UE_TWO_PI;
		const float Speed = (60.f + FMath::FRand() * 240.f) * KPDirectorConst::ToyPx;
		const float Side = FMath::FRand() < 0.5f ? 1.f : -1.f;
		FVector Velocity(FMath::Cos(Angle) * Speed, FMath::Sin(Angle) * Speed, 0.f);
		if (bHasNormal)
		{
			// Mostly out of both sides of the cut
			Velocity = Velocity * 0.3f + Normal * Side * Speed * 0.8f;
		}
		Velocity.Z = FMath::FRandRange(150.f, 600.f);

		FParticle Particle;
		Particle.Pos = At;
		Particle.Vel = Velocity;
		Particle.Life = 0.4f + FMath::FRand() * 0.6f;
		Particle.Size = (bBlood ? 1.4f + FMath::FRand() * 2.6f : 2.5f + FMath::FRand() * 3.5f) * KPDirectorConst::ToyPx;
		Particle.Kind = bBlood ? (FMath::FRand() < 0.55f ? Blood : DarkBlood) : Fluff;
		Particles.Add(Particle);
	}
}

void AKPDirector::Spray(const FVector& At, const FVector& Normal, bool bBlood)
{
	if (!BurstSystem)
	{
		return;
	}
	// One fan of streaks out of each side of the cut (the system sprays along its local X, tilted up)
	// (its sprite material, M_KPSprite, is opaque and corrects for the night exposure, so this is the colour seen)
	const FLinearColor Color = bBlood ? KPVisuals::Hex(TEXT("#B5102B")) : KPVisuals::Hex(TEXT("#FFF8EE"));
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FVector Direction = Side == 0 ? Normal : -Normal;
		UNiagaraComponent* Fx = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BurstSystem, At, Direction.Rotation(),
			FVector(1.f), true, true, ENCPoolMethod::AutoRelease);
		if (Fx)
		{
			Fx->SetVariableLinearColor(TEXT("User.Color"), Color);
		}
	}
}

void AKPDirector::Stain(const FVector& At, float Size, bool bSplat)
{
	// Splats (under a snip) and stains (where a drop lands) are kept apart so that the many small
	// stains can never push a splat out: splats stay on the mat for the whole night.
	UInstancedStaticMeshComponent* Ism = bSplat ? SplatIsm : StainIsm;
	int32& Cursor = bSplat ? SplatCursor : StainCursor;
	const int32 MaxCount = FMath::Max(1, bSplat ? Tuning.MaxSplats : Tuning.MaxStains);
	const float Z = KPDirectorConst::StainZ + (Cursor % 32) * 0.01f;
	const FTransform Transform(FRotator(0.f, FMath::FRand() * 360.f, 0.f), FVector(At.X, At.Y, Z),
		FVector(2.f * Size / 100.f, 2.f * Size / 100.f * FMath::FRandRange(0.8f, 1.15f), 0.004f));
	if (Ism->GetInstanceCount() < MaxCount)
	{
		Ism->AddInstance(Transform, true);
	}
	else
	{
		Ism->UpdateInstanceTransform(Cursor % MaxCount, Transform, true, true, true);
	}
	++Cursor;
}

void AKPDirector::StainSplat(const FVector& At, float Radius, const FVector& Normal)
{
	Stain(At, Radius * 0.85f, true);
	for (int32 Index = 0; Index < 8; ++Index)
	{
		const float Side = FMath::FRand() < 0.5f ? 1.f : -1.f;
		const float Along = Radius * (0.6f + FMath::FRand() * 1.8f);
		const FVector Jitter(FMath::FRandRange(-0.4f, 0.4f) * Radius, FMath::FRandRange(-0.4f, 0.4f) * Radius, 0.f);
		Stain(At + Normal * Side * Along + Jitter, Radius * (0.12f + FMath::FRand() * 0.3f), true);
	}
}

void AKPDirector::UpdateFx(float RealDt, float GameDt)
{
	// Particles: drag like the toy, plus gravity; blood that lands stains the mat
	const float Drag = FMath::Pow(0.02f, GameDt);
	for (int32 Index = Particles.Num() - 1; Index >= 0; --Index)
	{
		FParticle& Particle = Particles[Index];
		Particle.Pos += Particle.Vel * GameDt;
		Particle.Vel.X *= Drag;
		Particle.Vel.Y *= Drag;
		Particle.Vel.Z -= Tuning.BloodGravity * GameDt;
		Particle.Life -= GameDt;
		const bool bLanded = Particle.Pos.Z <= 0.f;
		if (bLanded || Particle.Life <= 0.f)
		{
			if (Particle.Kind != Fluff)
			{
				Stain(Particle.Pos, Particle.Size * 0.9f, false);
			}
			Particles.RemoveAtSwap(Index);
		}
	}

	for (int32 Index = Trail.Num() - 1; Index >= 0; --Index)
	{
		Trail[Index].Life -= RealDt;
		if (Trail[Index].Life <= 0.f)
		{
			Trail.RemoveAtSwap(Index);
		}
	}

	for (int32 Index = Texts.Num() - 1; Index >= 0; --Index)
	{
		Texts[Index].Life -= RealDt;
		if (Texts[Index].Life <= 0.f)
		{
			Texts.RemoveAt(Index);
		}
	}

	// Push to the instanced meshes
	TArray<FTransform> PerKind[NumKinds];
	for (const FParticle& Particle : Particles)
	{
		const float Scale = Particle.Size * 2.f / 100.f;
		PerKind[Particle.Kind].Add(FTransform(FRotator::ZeroRotator, Particle.Pos, FVector(Scale)));
	}
	for (int32 Kind = 0; Kind < NumKinds; ++Kind)
	{
		SetInstances(ParticleIsms[Kind], PerKind[Kind]);
	}

	TArray<FTransform> TrailTransforms;
	TrailTransforms.Reserve(Trail.Num());
	for (const FTrailDot& Dot : Trail)
	{
		// T7b: thins and fades towards its old end (the newest dots are full width)
		const float Fresh = FMath::Clamp(Dot.Life / KPDirectorConst::TrailLife, 0.f, 1.f);
		const float Radius = (Tuning.HeroRadius - 4.f) * FMath::Lerp(1.f - Tuning.TrailFade, 1.f, Fresh) * FMath::Sqrt(Fresh);
		TrailTransforms.Add(FTransform(FRotator::ZeroRotator, FVector(Dot.Pos.X, Dot.Pos.Y, KPDirectorConst::TrailZ),
			FVector(2.f * Radius / 100.f, 2.f * Radius / 100.f, 0.004f)));
	}
	SetInstances(TrailIsm, TrailTransforms);
}

void AKPDirector::AddWorldText(const FVector& At, const FString& Text, const FString& Sub, float Life, float Size, const FLinearColor& Color)
{
	FKPFloatText Entry;
	Entry.World = At;
	Entry.Text = Text;
	Entry.Sub = Sub;
	Entry.Life = Life;
	Entry.MaxLife = Life;
	Entry.Size = Size;
	Entry.Color = Color;
	Texts.Add(Entry);
}

void AKPDirector::AddScreenText(const FVector2D& At01, const FString& Text, float Life, float Size, const FLinearColor& Color)
{
	// A new text in exactly the same spot replaces the old one (quick purchases, two cards in a row)
	Texts.RemoveAll([&At01](const FKPFloatText& Each) { return Each.bOnScreen && Each.Screen.Equals(At01, 1e-4); });
	FKPFloatText Entry;
	Entry.bOnScreen = true;
	Entry.Screen = At01;
	Entry.Text = Text;
	Entry.Life = Life;
	Entry.MaxLife = Life;
	Entry.Size = Size;
	Entry.Color = Color;
	Texts.Add(Entry);
}

// ---------------------------------------------------------------------------
// Showing it

void AKPDirector::UpdatePreview()
{
	TArray<FTransform> Rings;
	TArray<FTransform> Warnings;
	TArray<FTransform> Safe;
	PreviewCount = 0;
	auto AddRing = [](TArray<FTransform>& Out, const FVector& Location, float RingRadius, float Width)
	{
		const float DashArc = UE_TWO_PI * RingRadius / (KPDirectorConst::RingDashes * 2);
		for (int32 Dash = 0; Dash < KPDirectorConst::RingDashes; ++Dash)
		{
			const float Angle = UE_TWO_PI * Dash / KPDirectorConst::RingDashes;
			const FVector Pos(Location.X + FMath::Cos(Angle) * RingRadius, Location.Y + FMath::Sin(Angle) * RingRadius, KPDirectorConst::RingZ);
			Out.Add(FTransform(FRotator(0.f, FMath::RadiansToDegrees(Angle) + 90.f, 0.f), Pos, FVector(DashArc / 100.f, Width / 100.f, 0.005f)));
		}
	};
	if (Mode == EKPMode::Draw && Path.Num() > 1)
	{
		// Pincushions the line crosses get a red warning ring; with Magnet, the first one a gold ring (safe)
		const AKPPincushion* SafePin = MagnetTarget();
		if (SafePin)
		{
			AddRing(Safe, SafePin->GetActorLocation(), SafePin->Radius + 8.f * KPDirectorConst::ToyPx, 13.f);
		}
		for (const AKPPincushion* Pin : Pins)
		{
			if (!IsValid(Pin) || Pin->bHit || Pin == SafePin)
			{
				continue;
			}
			const FVector Location = Pin->GetActorLocation();
			for (int32 Index = 0; Index < Path.Num() - 1; ++Index)
			{
				if (KPMath::SegmentDistance2D(Location, Path[Index], Path[Index + 1]) <= Pin->Radius + Tuning.BladePad)
				{
					AddRing(Warnings, Location, Pin->Radius + 8.f * KPDirectorConst::ToyPx, 13.f);
					break;
				}
			}
		}

		TArray<AKPPlushie*> InCircle;
		if (bExtraInch)
		{
			ExtraInchTargets(Path.Last(), InCircle);
		}
		for (AKPPlushie* Plushie : Plushies)
		{
			if (!IsValid(Plushie) || Plushie->bDead)
			{
				continue;
			}
			const FVector Location = Plushie->GetActorLocation();
			bool bOnLine = InCircle.Contains(Plushie);
			for (int32 Index = 0; Index < Path.Num() - 1 && !bOnLine; ++Index)
			{
				bOnLine = KPMath::SegmentDistance2D(Location, Path[Index], Path[Index + 1]) <= Plushie->Radius + Tuning.BladePad;
			}
			if (!bOnLine)
			{
				continue;
			}
			if (!(Types.IsValidIndex(Plushie->TypeIndex) && Types[Plushie->TypeIndex].ScoreMul <= 0.f))
			{
				++PreviewCount; // T7c: chicks (scoreMul 0) don't count in the combo
			}

			// A dashed ring on the mat around it
			AddRing(Rings, Location, Plushie->Radius + 5.f * KPDirectorConst::ToyPx, 6.f);
		}
	}
	// T7c: where a blinking pincushion will appear: a faint ring that pulses (grows and thins) BlinkPulses times
	// T7d: in the dark the ring glows purple and a small light pulses on the spot with each pulse (and tick)
	const float BlinkWarn = FMath::Max(0.1f, Tuning.BlinkWarn);
	const int32 BlinkPulses = FMath::Max(1, Tuning.BlinkPulses);
	TArray<FTransform> BlinkRings;
	int32 LightsUsed = 0;
	for (const AKPPincushion* Pin : Pins)
	{
		if (!IsValid(Pin) || !Pin->bBlinkAiming)
		{
			continue;
		}
		const float Into = FMath::Clamp((BlinkWarn - Pin->BlinkTimer) / BlinkWarn, 0.f, 1.f) * BlinkPulses;
		const float Pulse = FMath::Fractional(Into);
		AddRing(bLightsOut ? BlinkRings : Rings, Pin->BlinkTo, Pin->Radius * (0.7f + 0.5f * Pulse), FMath::Lerp(bLightsOut ? 9.f : 7.f, 2.f, Pulse));
		if (bLightsOut && BlinkLights.IsValidIndex(LightsUsed))
		{
			UPointLightComponent* Light = BlinkLights[LightsUsed++];
			Light->SetWorldLocation(Pin->BlinkTo + FVector(0.f, 0.f, 90.f));
			Light->SetIntensity(Tuning.BlinkLight * (0.25f + 0.75f * FMath::Square(1.f - Pulse)));
			Light->SetVisibility(true);
		}
	}
	for (int32 Index = LightsUsed; Index < BlinkLights.Num(); ++Index)
	{
		if (BlinkLights[Index]->IsVisible())
		{
			BlinkLights[Index]->SetVisibility(false);
		}
	}
	SetInstances(BlinkRingIsm, BlinkRings);
	SetInstances(RingIsm, Rings);
	SetInstances(WarnIsm, Warnings);
	SetInstances(SafeIsm, Safe);
}

void AKPDirector::UpdateLineVisuals()
{
	TArray<FTransform> Dashes;
	const bool bShowLine = Path.Num() > 1 && (Mode == EKPMode::Draw || Mode == EKPMode::Dash || Mode == EKPMode::Snip);
	if (bShowLine)
	{
		// Cumulative distances so dashes can be placed by distance along the line
		TArray<float> Along;
		Along.SetNum(Path.Num());
		Along[0] = 0.f;
		for (int32 Index = 1; Index < Path.Num(); ++Index)
		{
			Along[Index] = Along[Index - 1] + FVector::Dist2D(Path[Index - 1], Path[Index]);
		}
		const float Total = Along.Last();
		int32 Segment = 0;
		auto PointAt = [&](float S) -> FVector
		{
			while (Segment < Path.Num() - 2 && Along[Segment + 1] < S)
			{
				++Segment;
			}
			const float Length = FMath::Max(Along[Segment + 1] - Along[Segment], 1e-4f);
			return FMath::Lerp(Path[Segment], Path[Segment + 1], FMath::Clamp((S - Along[Segment]) / Length, 0.f, 1.f));
		};

		for (float S = 0.f; S < Total; S += KPDirectorConst::DashLength + KPDirectorConst::GapLength)
		{
			const FVector P0 = PointAt(S);
			const FVector P1 = PointAt(FMath::Min(S + KPDirectorConst::DashLength, Total));
			const float Length = FVector::Dist2D(P0, P1);
			if (Length < 1.f)
			{
				continue;
			}
			const FVector Mid = (P0 + P1) * 0.5f;
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(P1.Y - P0.Y, P1.X - P0.X));
			Dashes.Add(FTransform(FRotator(0.f, Yaw, 0.f), FVector(Mid.X, Mid.Y, KPDirectorConst::ChalkZ),
				FVector(Length / 100.f, KPDirectorConst::LineWidth / 100.f, 0.005f)));
		}

		// A chalk dot at the end of the line
		const FVector End = Path.Last();
		Dashes.Add(FTransform(FRotator(0.f, 45.f, 0.f), FVector(End.X, End.Y, KPDirectorConst::ChalkZ), FVector(0.17f, 0.17f, 0.006f)));

		// Extra inch: a dashed chalk cone ahead of the line's far end
		FVector ConeTip;
		FVector ConeDir;
		float ConeHalf = 0.f;
		float ConeLength = 0.f;
		if (ExtraInchCone(ConeTip, ConeDir, ConeHalf, ConeLength))
		{
			auto DashedSegment = [&Dashes](const FVector& A, const FVector& B)
			{
				const float Length = FVector::Dist2D(A, B);
				const int32 Count = FMath::Max(1, FMath::RoundToInt(Length / 26.f));
				for (int32 Dash = 0; Dash < Count; ++Dash)
				{
					const FVector P0 = FMath::Lerp(A, B, (Dash + 0.15f) / Count);
					const FVector P1 = FMath::Lerp(A, B, (Dash + 0.75f) / Count);
					const FVector Mid = (P0 + P1) * 0.5f;
					const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(P1.Y - P0.Y, P1.X - P0.X));
					Dashes.Add(FTransform(FRotator(0.f, Yaw, 0.f), FVector(Mid.X, Mid.Y, KPDirectorConst::ChalkZ),
						FVector(FVector::Dist2D(P0, P1) / 100.f, KPDirectorConst::LineWidth * 0.8f / 100.f, 0.005f)));
				}
			};
			const float BaseYaw = FMath::Atan2(ConeDir.Y, ConeDir.X);
			auto Rim = [&](float Angle) { return ConeTip + FVector(FMath::Cos(BaseYaw + Angle), FMath::Sin(BaseYaw + Angle), 0.f) * ConeLength; };
			DashedSegment(ConeTip, Rim(-ConeHalf));
			DashedSegment(ConeTip, Rim(ConeHalf));
			constexpr int32 ArcSteps = 4;
			for (int32 Step = 0; Step < ArcSteps; ++Step)
			{
				DashedSegment(Rim(-ConeHalf + 2.f * ConeHalf * Step / ArcSteps), Rim(-ConeHalf + 2.f * ConeHalf * (Step + 1) / ArcSteps));
			}
		}

		// Pink when the chalk is nearly used up
		const bool bNearlyOut = PathLength > Tuning.Chalk * 0.9f;
		KPVisuals::SetColor(ChalkMaterial, bNearlyOut ? KPVisuals::Hex(TEXT("#FFC2D4")) : FLinearColor::White);
	}
	SetInstances(ChalkIsm, Dashes);

	// Pinking shears: while drawing, a pale band along the line as wide as its reach (a plushie whose body
	// touches the band gets cut)
	TArray<FTransform> Band;
	if (Mode == EKPMode::Draw && Path.Num() > 1 && Tuning.BladePad > BaseTuning.BladePad + 0.5f)
	{
		const float Width = 2.f * Tuning.BladePad;
		for (int32 Index = 0; Index + 1 < Path.Num(); ++Index)
		{
			const FVector A = Path[Index];
			const FVector B = Path[Index + 1];
			const float Length = FVector::Dist2D(A, B);
			if (Length < 0.5f)
			{
				continue;
			}
			const FVector Mid = (A + B) * 0.5f;
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X));
			// A little longer than the segment so the joints close
			Band.Add(FTransform(FRotator(0.f, Yaw, 0.f), FVector(Mid.X, Mid.Y, KPDirectorConst::StainZ + 0.35f),
				FVector((Length + Width * 0.3f) / 100.f, Width / 100.f, 0.003f)));
		}
	}
	SetInstances(BandIsm, Band);
}

void AKPDirector::UpdateHero(float RealDt)
{
	AKPHeroine* H = Hero.Get();
	if (!H)
	{
		return;
	}

	if (Mode == EKPMode::Draw && Path.Num() > 1)
	{
		const FVector ToEnd = Path.Last() - HeroPos;
		if (ToEnd.Size2D() > 30.f)
		{
			HeroYaw = FMath::RadiansToDegrees(FMath::Atan2(ToEnd.Y, ToEnd.X));
		}
	}
	H->Place(HeroPos, HeroYaw);

	// Shears: appear as the dash starts and snap open, stay open (opening a touch wider) over the beat, and
	// snap shut in its last few hundredths of a second, so they close exactly as sfx_snip plays (DoSnip);
	// T7d: they stay a moment after, closed
	constexpr float ShearsOpen = 0.48f;
	constexpr float ShearsCloseTime = 0.05f;
	ShearsAfterSnip = FMath::Max(0.f, ShearsAfterSnip - RealDt);
	const bool bShears = Mode == EKPMode::Dash || Mode == EKPMode::Snip || ShearsAfterSnip > 0.f || IsWardrobe();
	float Open = IsWardrobe() ? 0.35f : 0.f;
	if (Mode == EKPMode::Dash)
	{
		Open = ShearsOpen * FMath::Clamp(DashTime / 0.06f, 0.f, 1.f);
	}
	else if (Mode == EKPMode::Snip)
	{
		const float Into = 1.f - FMath::Clamp(SnipTimer / FMath::Max(Tuning.Beat, 0.01f), 0.f, 1.f);
		Open = SnipTimer > ShearsCloseTime ? ShearsOpen * (1.f + 0.15f * FMath::Sin(Into * UE_HALF_PI))
			: ShearsOpen * 1.15f * FMath::Max(0.f, SnipTimer) / ShearsCloseTime;
	}
	H->SetShearsVisible(bShears);
	H->SetShearsOpen(Open);

	// She runs on real time: the world is slowed, she isn't
	H->CustomTimeDilation = 1.f / FMath::Max(Ts, 0.01f);
	H->SetBlinkHidden(Invulnerable > 0.f && FMath::FloorToInt(Invulnerable * 14.f) % 2 == 0);
	H->UpdateAnimation();
	H->Animate(RealDt); // T7f: her procedural life (real time)
}

void AKPDirector::UpdateCamera()
{
	ACameraActor* Cam = Camera.Get();
	if (!Cam)
	{
		return;
	}
	FVector Location = CameraBase.GetLocation();
	if (Shake > 0.f)
	{
		const FQuat Rotation = CameraBase.GetRotation();
		const float Amount = Shake * Tuning.ShakeUnits;
		Location += Rotation.GetRightVector() * FMath::FRandRange(-0.5f, 0.5f) * Amount;
		Location += Rotation.GetUpVector() * FMath::FRandRange(-0.5f, 0.5f) * Amount;
	}
	Cam->SetActorLocation(Location);

	// T7d: the camera cut ends when the beat (and the snip's hitstop) is over, or when the finale's slow
	// motion ends: back to the game camera before time returns to normal
	if (CineMode == 1 && Mode != EKPMode::Dash && Mode != EKPMode::Snip && HitStopTimer <= 0.f)
	{
		CineMode = 0;
	}
	if (CineMode == 2 && (FinaleTimer <= 0.f || !CineBoss.IsValid()))
	{
		CineMode = 0;
	}
	if (bAttract && CineMode != 0)
	{
		CineMode = 0;
	}
	ACameraActor* Close = CineCam.Get();
	if (!Close || (CineMode == 0 && !bCloseUpTest && !IsWardrobe()))
	{
		bCineSnap = true;
		return;
	}
	const float Dt = FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 1.f / 30.f);
	FVector WantLoc;
	FVector WantLook;
	const FVector HerFront(FMath::Cos(FMath::DegreesToRadians(HeroYaw)), FMath::Sin(FMath::DegreesToRadians(HeroYaw)), 0.f);
	if (CineMode == 1)
	{
		// Low, behind and beside her, looking along the line: a tracking shot of the dash, then the snip
		WantLoc = HeroPos - CineDir * 520.f + CineSide * 280.f + FVector(0.f, 0.f, 300.f);
		WantLook = HeroPos + CineDir * 260.f + FVector(0.f, 0.f, 110.f);
	}
	else if (CineMode == 2)
	{
		// The boss's final split: low, from her side of it
		const AKPPlushie* Boss = CineBoss.Get();
		const FVector BossAt = Boss->GetActorLocation();
		FVector Away = HeroPos - BossAt;
		Away.Z = 0.f;
		Away = Away.IsNearlyZero() ? FVector(-1.f, 0.f, 0.f) : Away.GetSafeNormal();
		const float R = FMath::Max(Boss->Radius, 80.f);
		WantLoc = BossAt + Away * R * 3.4f + FVector(Away.Y, -Away.X, 0.f) * R * 1.2f + FVector(0.f, 0.f, R * 1.1f);
		WantLook = BossAt + FVector(0.f, 0.f, R * 0.7f);
	}
	else
	{
		// The wardrobe and kp.CloseUp: in front of her, a little to her side, her whole figure in the frame
		const FVector Side(-HerFront.Y, HerFront.X, 0.f);
		WantLoc = HeroPos + HerFront * 520.f + Side * 60.f + FVector(0.f, 0.f, 230.f);
		WantLook = HeroPos + FVector(0.f, 0.f, 135.f) + Side * (IsWardrobe() ? -120.f : 0.f);
	}
	if (bCineSnap)
	{
		CineLoc = WantLoc;
		CineLook = WantLook;
		bCineSnap = false;
	}
	else
	{
		CineLoc = FMath::VInterpTo(CineLoc, WantLoc, Dt, 12.f);
		CineLook = FMath::VInterpTo(CineLook, WantLook, Dt, 16.f);
	}
	FVector Jolt = FVector::ZeroVector;
	if (Shake > 0.f)
	{
		Jolt = FVector(FMath::FRandRange(-0.5f, 0.5f), FMath::FRandRange(-0.5f, 0.5f), FMath::FRandRange(-0.5f, 0.5f)) * Shake * Tuning.ShakeUnits * 0.6f;
	}
	Close->SetActorLocationAndRotation(CineLoc + Jolt, (CineLook - CineLoc).Rotation());
}

int32 AKPDirector::CountOnPath() const
{
	int32 Count = 0;
	for (const AKPPlushie* Plushie : Plushies)
	{
		if (!IsValid(Plushie) || Plushie->bDead || (Types.IsValidIndex(Plushie->TypeIndex) && Types[Plushie->TypeIndex].ScoreMul <= 0.f && !Plushie->bBoss))
		{
			continue;
		}
		const FVector Location = Plushie->GetActorLocation();
		for (int32 Index = 0; Index + 1 < Path.Num(); ++Index)
		{
			if (KPMath::SegmentDistance2D(Location, Path[Index], Path[Index + 1]) <= Plushie->Radius + Tuning.BladePad)
			{
				++Count;
				break;
			}
		}
	}
	return Count;
}

void AKPDirector::StartCine(int32 InMode)
{
	if (bAttract || !AreCameraCutsOn() || !CineCam.IsValid())
	{
		return;
	}
	CineMode = InMode;
	bCineSnap = true;
	if (InMode == 1 && Path.Num() > 1)
	{
		FVector Dir = Path.Last() - Path[0];
		Dir.Z = 0.f;
		CineDir = Dir.IsNearlyZero() ? FVector(1.f, 0.f, 0.f) : Dir.GetSafeNormal();
		// The side nearer the game camera (-X), so the shot stays roughly the way the player sees the mat
		CineSide = FVector(-CineDir.Y, CineDir.X, 0.f);
		if (CineSide.X > 0.f)
		{
			CineSide = -CineSide;
		}
	}
	UE_LOG(LogKP, Log, TEXT("Camera cut (%d)"), InMode);
}

void AKPDirector::SetInstances(UInstancedStaticMeshComponent* Ism, const TArray<FTransform>& Transforms)
{
	if (!Ism)
	{
		return;
	}
	if (Ism->GetInstanceCount() != Transforms.Num())
	{
		Ism->ClearInstances();
		if (Transforms.Num() > 0)
		{
			Ism->AddInstances(Transforms, false, true, false);
		}
	}
	else if (Transforms.Num() > 0)
	{
		Ism->BatchUpdateInstancesTransforms(0, Transforms, true, true, true);
	}
}

// ---------------------------------------------------------------------------
// HUD access

float AKPDirector::GetChalkLeft01() const
{
	if (Mode != EKPMode::Draw)
	{
		return 1.f;
	}
	return FMath::Clamp(1.f - PathLength / FMath::Max(Tuning.Chalk, 1.f), 0.f, 1.f);
}

bool AKPDirector::GetLineEnd(FVector& OutEnd) const
{
	if (Mode == EKPMode::Draw && Path.Num() > 1)
	{
		OutEnd = Path.Last();
		return true;
	}
	return false;
}

void AKPDirector::CreateHud()
{
	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}
	const TWeakObjectPtr<AKPDirector> Weak(this);
	Hud = SNew(SKPHud).Director(Weak);
	// Under the HUD, a blur of the mat (pause, a new plushie's introduction); strength 0 draws nothing
	HudRoot = SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot()
		[
			SNew(SBackgroundBlur)
			.Visibility(EVisibility::HitTestInvisible)
			.BlurStrength_Lambda([Weak]() { return Weak.IsValid() ? Weak->GetBlur() * 9.f : 0.f; })
		]
		+ SOverlay::Slot()
		[
			Hud.ToSharedRef()
		];
	HudHost = SNew(SWeakWidget).PossiblyNullContent(HudRoot);
	Viewport->AddViewportWidgetContent(HudHost.ToSharedRef(), 10);
}

void AKPDirector::RemoveHud()
{
	if (HudHost.IsValid())
	{
		UWorld* World = GetWorld();
		if (UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(HudHost.ToSharedRef());
		}
	}
	HudHost.Reset();
	HudRoot.Reset();
	Hud.Reset();
}

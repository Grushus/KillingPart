// Killing Part: the menus (T6). The title screen runs the game in an attract mode behind its menu:
// plushies of every type wander in and she cuts them by herself (no hearts, no score), which also gets
// their materials, the cap and the blood effects ready before the first real night. The HUD paints the
// menus and records their buttons; clicks come back here as action strings (HandleUiAction).

#include "KillingPart/KPDirector.h"
#include "KillingPart/KPHeroine.h"
#include "KillingPart/KPPlayerController.h"
#include "KillingPart/KPPlushie.h"
#include "KillingPart/KPPortrait.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "PipelineStateCache.h"
#include "ShaderPipelineCache.h"
#include "UnrealClient.h"

void AKPDirector::GoToTitle()
{
	// A fresh mat (it also clears the last night), then the menu on top
	EndPractice(); // T8: the real save comes back (with any settings changed meanwhile)
	ModeIndex = 0;
	StartNight(0);
	bAttract = true;
	Countdown = 0.f;
	TutorialStep = 0;
	Texts.Reset();
	SetLightsOut(false);
	TitlePage = (Save && !Save->CutChosen) ? 1 : 0;
	if (TitlePage == 1)
	{
		// First launch: no blood behind the question until a cut is chosen
		KPSettings::SetTheatrical(true);
	}
	ApplyCut(false);
	AttractSpawnTimer = 0.2f;
	AutoCutTimer = 1.6f;
	UiClock = 0.f;
	// Two pincushions on the mat (their look gets ready too)
	SpawnPin();
	SpawnPin();
	UE_LOG(LogKP, Log, TEXT("Title screen (page %d)"), TitlePage);
}

void AKPDirector::PlayNight(int32 InNightIndex)
{
	bAttract = false;
	TitlePage = 0;
	AutoCutTimer = 0.5f;
	StartNight(InNightIndex);
}

void AKPDirector::UpdateAttract(float GameDt)
{
	// Keep a few plushies coming in from the edges, slowly, from every type in the title mix
	AttractSpawnTimer -= GameDt;
	int32 Alive = 0;
	for (const AKPPlushie* Plushie : Plushies)
	{
		Alive += IsValid(Plushie) && !Plushie->bDead ? 1 : 0;
	}
	if (AttractSpawnTimer > 0.f || Alive >= Tuning.TitlePlushies)
	{
		return;
	}
	AttractSpawnTimer = Tuning.TitleSpawnEvery;
	float Total = 0.f;
	for (const TPair<FString, float>& Entry : Tuning.TitleMix)
	{
		Total += FindType(Entry.Key) != INDEX_NONE ? FMath::Max(0.f, Entry.Value) : 0.f;
	}
	int32 TypeIndex = FindType(TEXT("teddy"));
	float Roll = FMath::FRand() * Total;
	for (const TPair<FString, float>& Entry : Tuning.TitleMix)
	{
		const int32 Index = FindType(Entry.Key);
		if (Index == INDEX_NONE || Entry.Value <= 0.f)
		{
			continue;
		}
		Roll -= Entry.Value;
		if (Roll <= 0.f)
		{
			TypeIndex = Index;
			break;
		}
	}
	if (AKPPlushie* Plushie = SpawnPlushie(FMath::Max(0, TypeIndex)))
	{
		Plushie->Speed = Tuning.TitleSpeed * Types[Plushie->TypeIndex].SpeedMul;
	}
}

void AKPDirector::UpdateTitleAuto(float RealDt)
{
	// -KPAutoCut -KPTitle: a screenshot of the title, then the first cut choice, Play and Normal, by itself
	if (!bTitleTest || !bAutoCut)
	{
		return;
	}
	const float Before = UiClock - RealDt;
	if (UiClock >= 3.f && Before < 3.f && AutoShotsLeft > 0)
	{
		--AutoShotsLeft;
		const FString File = FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/KP/Auto_%02d_title%d.png"), ++AutoShotIndex, TitlePage);
		FScreenshotRequest::RequestScreenshot(File, true, false);
	}
	if (UiClock >= 4.f)
	{
		const FKPModeSave* Normal = GetModeSave(0);
		HandleUiAction(TitlePage == 1 ? FString(TEXT("choose:director")) : TitlePage == 0 ? FString(TEXT("play"))
			: (Normal && Normal->Active) ? FString(TEXT("continue:0")) : FString(TEXT("new:0")));
		UiClock = 0.f;
	}
}

bool AKPDirector::IsPreparing() const
{
	// PSO precaching and a bundled pipeline cache both compile in the background on the first launch
	return PipelineStateCache::NumActivePrecacheRequests() > 0 || FShaderPipelineCache::NumPrecompilesRemaining() > 0 || RealClock < 0.5f;
}

void AKPDirector::QuitToDesktop()
{
	SaveProgress();
	UKismetSystemLibrary::QuitGame(this, PC.Get(), EQuitPreference::Quit, false);
}

void AKPDirector::SetPaused(bool bOn)
{
	if (bOn == bPaused)
	{
		return;
	}
	bPaused = bOn;
	UiClock = 0.f;
	// The world stops: plushies, physics halves, particles, her; the time scale (slow motion) is kept
	UGameplayStatics::SetGamePaused(this, bOn);
	UE_LOG(LogKP, Log, TEXT("%s"), bOn ? TEXT("Paused") : TEXT("Resumed"));
}

void AKPDirector::UpdatePauseInput()
{
	if (!PC.IsValid())
	{
		return;
	}
	// Esc (P too: in the editor Esc stops Play)
	const bool bToggle = PC->WasInputKeyJustPressed(EKeys::Escape) || PC->WasInputKeyJustPressed(EKeys::P);
	if (bAttract)
	{
		if (bToggle && TitlePage == 4)
		{
			GoToTitle(); // T7d: out of the wardrobe (the mat comes back to life)
		}
		else if (bToggle && TitlePage >= 2)
		{
			TitlePage = 0; // the modes: back to the menu
		}
		return;
	}
	if (bToggle && Mode != EKPMode::Over && Mode != EKPMode::Dawn)
	{
		SetPaused(!bPaused);
		return;
	}
	if (bPaused)
	{
		const bool bHeld = PC->IsCutHeld();
		const bool bPressed = bHeld && !bWasHeld;
		bWasHeld = bHeld;
		if (bPressed)
		{
			HandleUiAction(UiActionUnderCursor());
		}
	}
}

void AKPDirector::UpdateTutorial(float RealDt)
{
	// T6b: the prompt only fades: a new one waits until the old one has faded out, then fades in
	const FString Wanted = TutorialWanted();
	if (Wanted != TutorialShown)
	{
		TutorialFade = TutorialShown.IsEmpty() ? 0.f : TutorialFade - RealDt * 5.f;
		if (TutorialFade <= 0.f)
		{
			TutorialFade = 0.f;
			TutorialShown = Wanted;
		}
	}
	else if (!TutorialShown.IsEmpty())
	{
		TutorialFade = FMath::Min(1.f, TutorialFade + RealDt * 4.f);
	}
	if (TutorialStep == 0)
	{
		return;
	}
	// 1 "hold and drag" until a line long enough to cut is drawn, 2 "let go" until the first cut
	// (a cancelled line goes back to 1), 3 "combo" for a few seconds, 4 on the first card pick
	if (TutorialStep == 1 && Mode == EKPMode::Draw && PathLength >= Tuning.MinLine)
	{
		TutorialStep = 2;
	}
	else if (TutorialStep == 2 && Mode == EKPMode::Play)
	{
		TutorialStep = 1;
	}
	else if (TutorialStep == 3)
	{
		TutorialTimer -= RealDt;
		if (TutorialTimer <= 0.f)
		{
			TutorialStep = 0;
			if (Save)
			{
				Save->TutorialCut = true;
				SaveProgress();
			}
			return;
		}
	}
}

FString AKPDirector::GetTutorialText() const
{
	return bPaused ? FString() : TutorialShown;
}

FString AKPDirector::TutorialWanted() const
{
	switch (TutorialStep)
	{
	case 1: return Wave >= 1 && (Mode == EKPMode::Play || Mode == EKPMode::Draw) ? Text(TEXT("tutorial1"), TEXT("Hold the left mouse button and drag to draw the cut")) : FString();
	case 2: return Mode == EKPMode::Draw ? Text(TEXT("tutorial2"), TEXT("Let go to cut")) : FString();
	case 3: return Mode != EKPMode::Pick ? Text(TEXT("tutorial3"), TEXT("Cut several in one line for a combo")) : FString();
	case 4: return Mode == EKPMode::Pick ? Text(TEXT("tutorial4"), TEXT("Clear a wave to get a card. Cards reset every night.")) : FString();
	default: return FString();
	}
}

void AKPDirector::FindNewcomers(int32 WaveNumber, TArray<FString>& Out) const
{
	const FKPNight& Night = CurrentNight();
	if (!Save || !Night.Waves.IsValidIndex(WaveNumber - 1))
	{
		return;
	}
	const FKPWave& Data = Night.Waves[WaveNumber - 1];
	// The pincushion first (it's on the mat when the wave starts)
	if (Data.Pins + ExtraPins > 0 && !Save->MetTypes.Contains(TEXT("pincushion")))
	{
		Out.Add(TEXT("pincushion"));
	}
	// T7c: the blinking pincushion, the first night (or round) it can turn up
	if (Data.Pins + ExtraPins > 0 && CanBlinkPinsNow(WaveNumber) && Tuning.BlinkChance > 0.f && !Save->MetTypes.Contains(TEXT("blinkpin")))
	{
		Out.Add(TEXT("blinkpin"));
	}
	// Then every type the wave can bring that has an introduction (the teddy, its cubs, the chicks and the
	// golden plushie have none: the tutorial covers the teddy, and a rare surprise stays a surprise)
	auto Consider = [&](const FString& Name)
	{
		const int32 Index = FindType(Name);
		if (Index != INDEX_NONE && !Types[Index].Intro.IsEmpty() && !Save->MetTypes.Contains(Types[Index].Name) && !Out.Contains(Types[Index].Name))
		{
			Out.Add(Types[Index].Name);
		}
	};
	const TMap<FString, float>& Mix = Data.Mix.Num() > 0 ? Data.Mix : Night.Mix;
	for (const TPair<FString, float>& Entry : Mix)
	{
		if (Entry.Value > 0.f)
		{
			Consider(Entry.Key);
		}
	}
	for (const TPair<FString, int32>& Entry : Data.Extra)
	{
		if (Entry.Value > 0)
		{
			Consider(Entry.Key);
		}
	}
	// T7d: the impostor, before the first wave that brings one
	if (bImpostorRoll && ImpostorRollWave == WaveNumber && !Save->MetTypes.Contains(TEXT("impostor")))
	{
		Out.Add(TEXT("impostor"));
	}
	// The boss, before its fight
	if (!Data.Boss.IsEmpty())
	{
		Consider(Data.Boss);
	}
}

bool AKPDirector::ShowNextIntro()
{
	if (IntroQueue.Num() == 0)
	{
		return false;
	}
	IntroKey = IntroQueue[0];
	IntroQueue.RemoveAt(0);
	PlaySound(TEXT("intro")); // T7b
	if (!Portrait)
	{
		// Far above the table, out of every shot but its own
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Portrait = GetWorld()->SpawnActor<AKPPortrait>(AKPPortrait::StaticClass(), FVector(0.f, 0.f, 20000.f), FRotator::ZeroRotator, Params);
	}
	FString Intro;
	bIntroBoss = false;
	if (IntroKey == TEXT("pincushion"))
	{
		Intro = Text(TEXT("introPincushion"), TEXT("Pincushion: Cutting through one costs a heart"));
		if (Portrait)
		{
			Portrait->ShowPincushion(Tuning.PinRadius);
		}
	}
	else if (IntroKey == TEXT("goldpin"))
	{
		// T8 test only (-KPCastShots): the golden pincushion's portrait for the cast picture
		Intro = TEXT("Golden pincushion: Cut it for a fortune");
		if (Portrait)
		{
			Portrait->ShowPincushion(Tuning.PinRadius, false, true);
		}
	}
	else if (IntroKey == TEXT("blinkpin"))
	{
		Intro = Text(TEXT("introBlinkPin"), TEXT("Blinking pincushion: Every few seconds it pops up somewhere else. Watch for the ring"));
		if (Portrait)
		{
			Portrait->ShowPincushion(Tuning.PinRadius, true);
		}
	}
	else
	{
		const int32 Index = FindType(IntroKey);
		if (Index == INDEX_NONE)
		{
			return ShowNextIntro();
		}
		const FKPPlushieType& Type = Types[Index];
		Intro = Type.Intro;
		bIntroBoss = Type.Boss;
		auto ColorOf = [](const FKPPlushieType& Each, const TCHAR* Fallback)
		{
			return KPVisuals::Hex(Each.Colors.Num() > 0 ? *Each.Colors[0] : Fallback);
		};
		// A hen comes with one of her chicks
		const int32 CompanionIndex = Type.Lays.IsEmpty() ? INDEX_NONE : FindType(Type.Lays);
		const FKPPlushieType* Companion = CompanionIndex != INDEX_NONE ? &Types[CompanionIndex] : nullptr;
		if (Portrait)
		{
			Portrait->ShowPlushie(Type, Index, (Type.RadiusMin + Type.RadiusMax) * 0.5f, ColorOf(Type, TEXT("#DCCBF3")),
				Companion, CompanionIndex, Companion ? Companion->RadiusMax : 0.f, Companion ? ColorOf(*Companion, TEXT("#FFE38A")) : FLinearColor::White);
		}
	}
	if (!Intro.Split(TEXT(": "), &IntroName, &IntroLine))
	{
		IntroName = Intro;
		IntroLine.Reset();
	}
	UiClock = 0.f;
	TsTarget = 1.f;
	UE_LOG(LogKP, Log, TEXT("Meet: %s"), *IntroKey);
	return true;
}

void AKPDirector::FinishIntro()
{
	if (Mode != EKPMode::Intro)
	{
		return;
	}
	if (Save)
	{
		Save->MetTypes.AddUnique(IntroKey);
		SaveProgress();
	}
	if (ShowNextIntro())
	{
		return;
	}
	if (Portrait)
	{
		Portrait->Clear();
	}
	Mode = EKPMode::Play;
	Cooldown = 0.25f;
	WaveTimer = 0.3f; // the wave starts right after
	if (bNightBannerPending)
	{
		// T6b: the night's first wave brought the introduction: now the banner (name and rule name only)
		bNightBannerPending = false;
		ShowNightBanner(false);
		Countdown = bCountdownAfterIntro ? Tuning.Countdown : 0.f; // T6c: then the 3-2-1 (T6d: only after Continue)
		bCountdownAfterIntro = false;
		WaveTimer = Tuning.FirstWaveDelay;
	}
}

UTextureRenderTarget2D* AKPDirector::GetPortraitTexture() const
{
	return Portrait ? Portrait->GetTexture() : nullptr;
}

void AKPDirector::HandleUiAction(const FString& Action)
{
	if (Action.IsEmpty())
	{
		return;
	}
	UE_LOG(LogKP, Log, TEXT("UI: %s"), *Action);
	// T7: the volume arrows (title and pause) and the sound test; every other button clicks
	if (Action.StartsWith(TEXT("st:")))
	{
		HandleSoundTestAction(Action);
		return;
	}
	if (HandleVolumeAction(Action))
	{
		return;
	}
	PlaySound(TEXT("click"));

	// The pause menu
	if (bPaused)
	{
		if (Action == TEXT("resume"))
		{
			SetPaused(false);
		}
		else if (Action == TEXT("togglecut"))
		{
			// Switches live, like kp.Cut (the Theatrical cut wipes the blood already on the mat)
			KPSettings::SetTheatrical(!KPSettings::IsTheatrical());
			ApplyCut(true);
			SaveProgress();
		}
		else if (Action == TEXT("togglecam"))
		{
			if (Save)
			{
				Save->CameraCuts = !Save->CameraCuts; // T7d
			}
			SaveProgress();
		}
		else if (Action == TEXT("restart"))
		{
			// Restart night (endless: restart round, from the round's save)
			BankPlayTime();
			SetPaused(false);
			StartNight(NightIndex);
			if (IsEndless())
			{
				ResumeEndlessRun();
			}
		}
		else if (Action == TEXT("title"))
		{
			// T6d: Save & quit to home (the week is saved at the start of every night, a run at every round)
			BankPlayTime();
			SaveProgress();
			SetPaused(false);
			GoToTitle();
		}
		else if (Action == TEXT("quit"))
		{
			// T6d: Save & quit game
			BankPlayTime();
			QuitToDesktop();
		}
		return;
	}

	// The title screen
	if (bAttract)
	{
		if (Action.StartsWith(TEXT("choose:")))
		{
			// The first launch's question: which version to play (also the content setting)
			KPSettings::SetTheatrical(Action.EndsWith(TEXT("theatrical")));
			if (Save)
			{
				Save->CutChosen = true;
			}
			ApplyCut(false);
			SaveProgress();
			TitlePage = 0;
			UiClock = 0.f;
		}
		else if (Action == TEXT("play") || Action == TEXT("modes"))
		{
			// T6c: Play opens the modes. T6d: two cards (the week modes, the endless modes); each first shows
			// the mode played last on its side, or else its furthest unlocked mode
			for (int32 Side = 0; Side < 2; ++Side)
			{
				TArray<int32> List;
				ModesOnSide(Side, List);
				if (List.Contains(ShownMode[Side]))
				{
					continue;
				}
				ShownMode[Side] = List.Num() > 0 ? List[0] : -1;
				for (const int32 Index : List)
				{
					if (Save && Modes[Index].Id == Save->SelectedMode)
					{
						ShownMode[Side] = Index;
						break;
					}
					if (IsModeUnlocked(Index))
					{
						ShownMode[Side] = Index;
					}
				}
			}
			TitlePage = 3;
			UiClock = 0.f;
		}
		else if (Action.StartsWith(TEXT("modeprev:")) || Action.StartsWith(TEXT("modenext:")))
		{
			StepShownMode(FCString::Atoi(*Action.RightChop(9)), Action.StartsWith(TEXT("modeprev:")) ? -1 : 1);
		}
		else if (Action.StartsWith(TEXT("new:")) || Action.StartsWith(TEXT("continue:")))
		{
			// A mode card's buttons (by index or id)
			const bool bNew = Action.StartsWith(TEXT("new:"));
			const FString Key = Action.RightChop(bNew ? 4 : 9);
			const int32 Index = Key.IsNumeric() ? FCString::Atoi(*Key) : Modes.IndexOfByPredicate([&Key](const FKPModeInfo& Each) { return Each.Id == Key; });
			if (Modes.IsValidIndex(Index) && IsModeUnlocked(Index))
			{
				if (bNew)
				{
					NewGame(Index);
				}
				else
				{
					ContinueGame(Index);
				}
			}
		}
		else if (Action.StartsWith(TEXT("practice:")))
		{
			// T8: Practice, under the mode cards: any night with Normal rules, nothing saved
			const int32 Night = FCString::Atoi(*Action.RightChop(9));
			if (Nights.IsValidIndex(Night))
			{
				StartPractice(Night);
			}
		}
		else if (Action == TEXT("back"))
		{
			if (TitlePage == 4)
			{
				GoToTitle(); // T7d: out of the wardrobe
				return;
			}
			TitlePage = 0;
			UiClock = 0.f;
		}
		else if (Action == TEXT("wardrobe"))
		{
			OpenWardrobe();
		}
		else if (Action.StartsWith(TEXT("wd:")) || Action.StartsWith(TEXT("wdprev:")) || Action.StartsWith(TEXT("wdnext:")))
		{
			// T7d: a swatch (wd:<row>:<index>) or a row's arrows
			TArray<FString> Parts;
			Action.ParseIntoArray(Parts, TEXT(":"));
			const int32 Row = Parts.Num() > 1 ? FCString::Atoi(*Parts[1]) : 0;
			TArray<FLinearColor> Swatches;
			WardrobeSwatches(Row, Swatches);
			if (Save && Swatches.Num() > 0)
			{
				int32 Pick = WardrobePick(Row);
				if (Parts[0] == TEXT("wd") && Parts.Num() > 2)
				{
					Pick = FCString::Atoi(*Parts[2]);
				}
				else
				{
					Pick += Parts[0] == TEXT("wdprev") ? -1 : 1;
				}
				Save->Wardrobe.Add(FString::FromInt(Row), (Pick % Swatches.Num() + Swatches.Num()) % Swatches.Num());
				SaveProgress();
				ApplyWardrobe();
			}
		}
		else if (Action == TEXT("togglecut"))
		{
			KPSettings::SetTheatrical(!KPSettings::IsTheatrical());
			ApplyCut(false);
			SaveProgress();
		}
		else if (Action == TEXT("togglecam"))
		{
			if (Save)
			{
				Save->CameraCuts = !Save->CameraCuts; // T7d
			}
			SaveProgress();
		}
		else if (Action == TEXT("quit"))
		{
			QuitToDesktop();
		}
		return;
	}

	// The results screen: the next night, the same night again, back to Monday, or (after Sunday) home
	if ((Mode != EKPMode::Over && Mode != EKPMode::Dawn) || UiClock < Tuning.UiClickDelay)
	{
		return;
	}
	if (Action == TEXT("next"))
	{
		if (Result.bWon && Result.bWeekDone)
		{
			GoToTitle();
		}
		else if (Result.bRunOver)
		{
			NewGame(ModeIndex);
		}
		else
		{
			StartNight(Result.bWon ? NightIndex + 1 : NightIndex);
		}
	}
	else if (Action == TEXT("title"))
	{
		GoToTitle();
	}
}

void AKPDirector::OpenWardrobe()
{
	// T7d: the mat is cleared, the room goes dark and her lamp comes on; the close camera looks at her
	ClearPlushies();
	Particles.Reset();
	Trail.Reset();
	Path.Reset();
	PathLength = 0.f;
	Mode = EKPMode::Play;
	TsTarget = 1.f;
	HeroPos = FVector::ZeroVector;
	HeroYaw = 180.f;
	TitlePage = 4;
	UiClock = 0.f;
	SetLightsOut(true);
	UE_LOG(LogKP, Log, TEXT("Wardrobe"));
}

FString AKPDirector::WardrobeRowName(int32 Row) const
{
	switch (Row)
	{
	case 0: return Text(TEXT("wardrobeDress"), TEXT("Dress"));
	case 1: return Text(TEXT("wardrobeApron"), TEXT("Apron"));
	case 2: return Text(TEXT("wardrobeHair"), TEXT("Hair and bow"));
	case 3: return Text(TEXT("wardrobeShears"), TEXT("Shears"));
	default: return Text(TEXT("wardrobeTrail"), TEXT("Dash trail"));
	}
}

namespace KPWardrobeDefaults
{
	const TCHAR* const Dress[] = { TEXT("#5B2A55"), TEXT("#2B3A67"), TEXT("#2F5D46"), TEXT("#26222B"), TEXT("#8C1F3A"), TEXT("#9C8AD0") };
	const TCHAR* const Apron[] = { TEXT("#F7F3EC"), TEXT("#FFD3E0"), TEXT("#CDEFD9"), TEXT("#FFF0B3"), TEXT("#CFE6FF"), TEXT("#4A4550") };
	const TCHAR* const Hair[] = { TEXT("#FF78B4"), TEXT("#B79CFF"), TEXT("#7FE0C0"), TEXT("#FFB38A"), TEXT("#E8354F"), TEXT("#FFE08A") };
	const TCHAR* const Bow[] = { TEXT("#FF3D8B"), TEXT("#7A4FE0"), TEXT("#2EB88F"), TEXT("#FF7A3D"), TEXT("#FFFFFF"), TEXT("#FF5C8A") };
	const TCHAR* const Shears[] = { TEXT("#FF4F9A"), TEXT("#FFC93C"), TEXT("#2EC4B6"), TEXT("#8E5BD6"), TEXT("#E0344F"), TEXT("#2A2730") };
	const TCHAR* const Trail[] = { TEXT("#FF78B4"), TEXT("#FFD23F"), TEXT("#5CE1E6"), TEXT("#B79CFF"), TEXT("#7FE0A0"), TEXT("#FFFFFF") };
}

void AKPDirector::WardrobeSwatches(int32 Row, TArray<FLinearColor>& Out) const
{
	Out.Reset();
	const TArray<FString>* FromJson = Row == 0 ? &WardrobeData.Dress : Row == 1 ? &WardrobeData.Apron : Row == 2 ? &WardrobeData.Hair
		: Row == 3 ? &WardrobeData.Shears : &WardrobeData.Trail;
	if (FromJson->Num() > 0)
	{
		for (const FString& Each : *FromJson)
		{
			Out.Add(KPVisuals::Hex(*Each));
		}
		return;
	}
	namespace W = KPWardrobeDefaults;
	const TCHAR* const* List = Row == 0 ? W::Dress : Row == 1 ? W::Apron : Row == 2 ? W::Hair : Row == 3 ? W::Shears : W::Trail;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Out.Add(KPVisuals::Hex(List[Index]));
	}
}

int32 AKPDirector::WardrobePick(int32 Row) const
{
	const int32* Found = Save ? Save->Wardrobe.Find(FString::FromInt(Row)) : nullptr;
	return Found ? FMath::Max(0, *Found) : 0;
}

FLinearColor AKPDirector::WardrobeColor(int32 Row, bool bSecond) const
{
	TArray<FLinearColor> Swatches;
	WardrobeSwatches(Row, Swatches);
	const int32 Pick = Swatches.Num() > 0 ? WardrobePick(Row) % Swatches.Num() : 0;
	if (bSecond)
	{
		// The bow, paired with the hair swatch
		if (WardrobeData.Bow.Num() > 0)
		{
			return KPVisuals::Hex(*WardrobeData.Bow[Pick % WardrobeData.Bow.Num()]);
		}
		return KPVisuals::Hex(KPWardrobeDefaults::Bow[Pick % 6]);
	}
	return Swatches.IsValidIndex(Pick) ? Swatches[Pick] : FLinearColor::White;
}

void AKPDirector::ApplyWardrobe()
{
	AKPHeroine* H = Hero.Get();
	if (!H || !Save)
	{
		return;
	}
	H->SetLook(WardrobeColor(0), WardrobeColor(1), WardrobeColor(2), WardrobeColor(2, true), WardrobeColor(3));
	KPVisuals::SetColor(TrailMaterial, WardrobeColor(4));
	bWardrobeApplied = true;
}

void AKPDirector::NewGame(int32 Index)
{
	if (!Modes.IsValidIndex(Index) || !Save)
	{
		return;
	}
	ModeIndex = Index;
	const FKPModeInfo& Info = Modes[Index];
	FKPModeSave& Week = Save->FindModeSave(Info.Id);
	Week.Active = true;
	Week.Night = 0;
	Week.Hearts = Info.Hearts > 0 ? Info.Hearts : BaseTuning.Hearts;
	Week.NightScores.Init(0, Nights.Num());
	Week.Time = 0.f;
	Week.Round = 0;
	Week.Score = 0;
	Week.Cards.Reset();
	RollDarkWaves(Week);
	Save->SelectedMode = Info.Id;
	ShownMode[Info.Endless ? 1 : 0] = Index;
	SaveProgress();
	UE_LOG(LogKP, Log, TEXT("New game: %s"), *Info.Name);
	PlayNight(0);
}

void AKPDirector::ContinueGame(int32 Index)
{
	const FKPModeSave* Week = GetModeSave(Index);
	if (!Week || !Week->Active || (Modes[Index].Endless && Week->Round < 1))
	{
		NewGame(Index);
		return;
	}
	ModeIndex = Index;
	if (Save)
	{
		Save->SelectedMode = Modes[Index].Id;
	}
	ShownMode[Modes[Index].Endless ? 1 : 0] = Index;
	// T6d: the 3-2-1 only comes after Continue (a loaded save)
	bCountdownNext = true;
	if (Modes[Index].Endless)
	{
		// T6d: an endless run starts its saved round again, with the hearts, cards and score it had then
		UE_LOG(LogKP, Log, TEXT("Continue: %s, round %d"), *Modes[Index].Name, Week->Round);
		PlayNight(0);
		ResumeEndlessRun();
		return;
	}
	UE_LOG(LogKP, Log, TEXT("Continue: %s, night %d"), *Modes[Index].Name, Week->Night + 1);
	PlayNight(Week->Night);
}

void AKPDirector::CopySettings(const UKPSaveGame* From, UKPSaveGame* To) const
{
	if (!From || !To)
	{
		return;
	}
	To->Theatrical = From->Theatrical;
	To->CutChosen = From->CutChosen;
	To->CameraCuts = From->CameraCuts;
	To->MusicVolume = From->MusicVolume;
	To->SfxVolume = From->SfxVolume;
	To->SoundPicks = From->SoundPicks;
	To->Wardrobe = From->Wardrobe;
}

void AKPDirector::StartPractice(int32 Night)
{
	if (!Save || !Nights.IsValidIndex(Night) || Modes.Num() == 0)
	{
		return;
	}
	// T8: the game plays on a copy of the save; the real one waits untouched (SaveProgress writes it with
	// the copy's settings only), and comes back in GoToTitle
	if (!bPractice)
	{
		PracticeBackup = Save;
		Save = DuplicateObject<UKPSaveGame>(PracticeBackup, this);
		bPractice = true;
	}
	ModeIndex = 0; // Normal's rules
	FKPModeSave& Week = Save->FindModeSave(Modes[0].Id);
	Week.Active = true;
	Week.Night = Night;
	Week.Hearts = Modes[0].Hearts > 0 ? Modes[0].Hearts : BaseTuning.Hearts;
	Week.NightScores.Init(0, Nights.Num());
	Week.Time = 0.f;
	RollDarkWaves(Week);
	UE_LOG(LogKP, Log, TEXT("Practice: %s (nothing is saved)"), *Nights[Night].Name);
	PlayNight(Night);
}

void AKPDirector::EndPractice()
{
	if (!bPractice)
	{
		return;
	}
	bPractice = false;
	if (PracticeBackup)
	{
		if (Save)
		{
			Save->Theatrical = KPSettings::IsTheatrical();
		}
		CopySettings(Save, PracticeBackup);
		Save = PracticeBackup;
	}
	PracticeBackup = nullptr;
	if (Save)
	{
		KPSave::Write(Save);
	}
	UE_LOG(LogKP, Log, TEXT("Practice over: the real save is back"));
}

void AKPDirector::BankPlayTime()
{
	FKPModeSave* Week = bAttract ? nullptr : ModeSaveMut();
	if (Week && Week->Active && PlayTime > 0.0)
	{
		Week->Time += static_cast<float>(PlayTime);
	}
	PlayTime = 0.0;
}

void AKPDirector::SaveEndlessRound()
{
	FKPModeSave* Run = (IsEndless() && Save) ? ModeSaveMut() : nullptr;
	if (!Run)
	{
		return;
	}
	Run->Active = true;
	BankPlayTime();
	Run->Round = Wave;
	Run->Score = Score;
	Run->Hearts = Hearts;
	Run->Cards.Reset();
	for (const int32 CardIndex : Held)
	{
		if (Cards.IsValidIndex(CardIndex))
		{
			Run->Cards.Add(Cards[CardIndex].Id);
		}
	}
	SaveProgress();
}

bool AKPDirector::ResumeEndlessRun()
{
	const FKPModeSave* Run = GetModeSave(ModeIndex);
	if (!IsEndless() || !Run || !Run->Active || Run->Round < 1)
	{
		return false;
	}
	Held.Reset();
	for (const FString& Id : Run->Cards)
	{
		const int32 CardIndex = Cards.IndexOfByPredicate([&Id](const FKPCard& Each) { return Each.Id == Id; });
		if (CardIndex != INDEX_NONE)
		{
			Held.Add(CardIndex);
		}
	}
	ApplyCards();
	Hearts = FMath::Clamp(Run->Hearts, 1, Tuning.MaxHearts);
	Score = Run->Score;
	EnsureEndlessRound(Run->Round);
	Wave = Run->Round - 1; // (the round starts after the usual delay, and saves itself again)
	UE_LOG(LogKP, Log, TEXT("Run resumed: round %d, score %d, hearts %d, %d cards"), Run->Round, Score, Hearts, Held.Num());
	return true;
}

void AKPDirector::ModesOnSide(int32 Side, TArray<int32>& Out) const
{
	Out.Reset();
	for (int32 Index = 0; Index < Modes.Num(); ++Index)
	{
		if (Modes[Index].Endless == (Side == 1))
		{
			Out.Add(Index);
		}
	}
}

int32 AKPDirector::GetShownMode(int32 Side) const
{
	TArray<int32> List;
	ModesOnSide(Side, List);
	if (List.Num() == 0)
	{
		return -1;
	}
	return Side >= 0 && Side < 2 && List.Contains(ShownMode[Side]) ? ShownMode[Side] : List[0];
}

void AKPDirector::StepShownMode(int32 Side, int32 Step)
{
	TArray<int32> List;
	ModesOnSide(Side, List);
	if (List.Num() == 0 || Side < 0 || Side > 1)
	{
		return;
	}
	const int32 At = FMath::Max(0, List.IndexOfByKey(GetShownMode(Side)));
	ShownMode[Side] = List[(At + Step + List.Num()) % List.Num()];
}

const FKPModeSave* AKPDirector::GetModeSave(int32 Index) const
{
	if (!Save || !Modes.IsValidIndex(Index))
	{
		return nullptr;
	}
	const FString& Id = Modes[Index].Id;
	return Save->ModeSaves.FindByPredicate([&Id](const FKPModeSave& Each) { return Each.Id == Id; });
}

FKPModeSave* AKPDirector::ModeSaveMut()
{
	return Save && Modes.IsValidIndex(ModeIndex) ? &Save->FindModeSave(Modes[ModeIndex].Id) : nullptr;
}

FString AKPDirector::ModeContinueLabel(int32 Index) const
{
	const FKPModeSave* Week = GetModeSave(Index);
	if (!Week || !Week->Active)
	{
		return FString();
	}
	if (Modes[Index].Endless)
	{
		// T6d: "Continue: Round 23, 41,200"
		return Week->Round < 1 ? FString() : Text(TEXT("continueRound"), TEXT("Continue: Round {round}, {score}"))
			.Replace(TEXT("{round}"), *FString::FromInt(Week->Round)).Replace(TEXT("{score}"), *FText::AsNumber(Week->Score).ToString());
	}
	if (!Nights.IsValidIndex(Week->Night))
	{
		return FString();
	}
	// T6d: "Continue: Tuesday, 10,050" (the week's score so far)
	return Text(TEXT("continue"), TEXT("Continue: {night}, {score}")).Replace(TEXT("{night}"), *Nights[Week->Night].Name)
		.Replace(TEXT("{score}"), *FText::AsNumber(Week->WeekScore()).ToString());
}

void AKPDirector::RollDarkWaves(FKPModeSave& Week) const
{
	// Exactly DarkWaves waves across the nights after the Lights out night, at most DarkPerNight in one
	// night, never a boss wave
	Week.DarkWaves.Reset();
	const int32 LightsOutNight = Nights.IndexOfByPredicate([](const FKPNight& Each) { return Each.Rule.Equals(TEXT("lightsout"), ESearchCase::IgnoreCase); });
	if (LightsOutNight == INDEX_NONE)
	{
		return;
	}
	TArray<int32> Candidates;
	for (int32 Night = LightsOutNight + 1; Night < Nights.Num(); ++Night)
	{
		for (int32 WaveNumber = 1; WaveNumber <= Nights[Night].Waves.Num(); ++WaveNumber)
		{
			if (Nights[Night].Waves[WaveNumber - 1].Boss.IsEmpty())
			{
				Candidates.Add(Night * 100 + WaveNumber);
			}
		}
	}
	for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
	{
		Candidates.Swap(Index, FMath::RandRange(0, Index));
	}
	TMap<int32, int32> PerNight;
	for (const int32 Each : Candidates)
	{
		if (Week.DarkWaves.Num() >= BaseTuning.DarkWaves)
		{
			break;
		}
		int32& Count = PerNight.FindOrAdd(Each / 100);
		if (Count < BaseTuning.DarkPerNight)
		{
			++Count;
			Week.DarkWaves.Add(Each);
		}
	}
	Week.DarkWaves.Sort();
	FString List;
	for (const int32 Each : Week.DarkWaves)
	{
		List += FString::Printf(TEXT(" %s/%d"), Nights.IsValidIndex(Each / 100) ? *Nights[Each / 100].Name.Left(3) : TEXT("?"), Each % 100);
	}
	UE_LOG(LogKP, Log, TEXT("Dark waves this week:%s"), *List);
}

bool AKPDirector::IsDarkWave(int32 Night, int32 WaveNumber) const
{
	const FKPModeSave* Week = bAttract ? nullptr : GetModeSave(ModeIndex);
	return Week && Week->DarkWaves.Contains(Night * 100 + WaveNumber);
}

int32 AKPDirector::AliveCount() const
{
	int32 Alive = 0;
	for (const AKPPlushie* Plushie : Plushies)
	{
		Alive += IsValid(Plushie) && !Plushie->bDead ? 1 : 0;
	}
	return Alive;
}

const FKPModeInfo& AKPDirector::CurrentMode() const
{
	static const FKPModeInfo Fallback;
	return Modes.IsValidIndex(ModeIndex) ? Modes[ModeIndex] : Fallback;
}

bool AKPDirector::IsModeUnlocked(int32 Index) const
{
	if (!Modes.IsValidIndex(Index))
	{
		return false;
	}
	const FString& Unlock = Modes[Index].Unlock;
	if (Unlock.IsEmpty() || bUnlockAll)
	{
		return true;
	}
	const int32 Before = Modes.IndexOfByPredicate([&Unlock](const FKPModeInfo& Each) { return Each.Id == Unlock; });
	const FKPModeSave* Week = GetModeSave(Before);
	if (Modes[Index].UnlockRound > 0)
	{
		return Week && Week->BestRound > Modes[Index].UnlockRound; // (Extended nightmare: round 50 finished in Extended cut, i.e. 51 reached)
	}
	return Week && Week->Finished;
}

bool AKPDirector::IsEndless() const
{
	return !bAttract && Modes.IsValidIndex(ModeIndex) && Modes[ModeIndex].Endless;
}

void AKPDirector::EnsureEndlessRound(int32 RoundNumber)
{
	// T6c: every round from formulas (KillingPart.json "endless"); nothing is written out by hand
	const FKPEndless& E = Endless;
	while (EndlessNight.Waves.Num() < RoundNumber)
	{
		const int32 R = EndlessNight.Waves.Num() + 1;
		FKPWave Round;
		Round.Speed = FMath::Min(E.SpeedMax, E.SpeedBase + E.SpeedStep * R);
		Round.Interval = FMath::Max(E.IntervalMin, E.IntervalStart - E.IntervalStep * R);
		Round.EscortEvery = 0.f;
		// A new type every NewTypeEvery rounds, in order; once all have joined, their shares keep growing
		const int32 Joined = E.NewTypeEvery > 0 ? R / E.NewTypeEvery : 0;
		const float Extra = E.HarderStep * FMath::Max(0, Joined - E.TypeOrder.Num());
		Round.Mix.Add(TEXT("teddy"), 1.f);
		int32 PinRound = 0;
		for (int32 Index = 0; Index < FMath::Min(Joined, E.TypeOrder.Num()); ++Index)
		{
			if (E.TypeOrder[Index] == TEXT("pincushion"))
			{
				PinRound = (Index + 1) * E.NewTypeEvery;
			}
			else
			{
				Round.Mix.Add(E.TypeOrder[Index], E.TypeWeight + Extra);
			}
		}
		Round.Pins = PinRound > 0 ? FMath::Min(E.PinsMax, 1 + (R - PinRound) / FMath::Max(1, E.PinsEvery)) : 0;
		const bool bBossRound = R >= E.BossFrom && E.BossEvery > 0 && (R - E.BossFrom) % E.BossEvery == 0;
		if (bBossRound)
		{
			Round.Count = 0;
			Round.Pins = 0;
			Round.Boss = E.BossType;
			Round.BossCount = 1 + (R - E.BossFrom) / FMath::Max(1, E.BossCountEvery);
			Round.BossHits = E.BossHitsBase + E.BossHitsStep * ((R - E.BossFrom) / E.BossEvery);
		}
		else
		{
			Round.Count = E.PlushiesBase + E.PlushiesPerRound * (R - 1);
		}
		EndlessNight.Waves.Add(Round);
	}
}

int32 AKPDirector::GetModeBest(int32 Index) const
{
	const FKPModeSave* Week = GetModeSave(Index);
	return Week ? Week->Record : 0;
}

// Killing Part: the week (T5, cut down to one screen in T5b). The save, each night's bests and the week
// score, and the results screen after every night (painted by SKPHud; its one button comes back here as
// an action string, HandleUiAction). T5b decision (Simon, Sat 20:32): no buttons, workbench or yard; with
// this little content they were detours from cutting.

#include "KillingPart/KPDirector.h"
#include "KillingPart/KPPlayerController.h"
#include "KillingPart/SKPHud.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void AKPDirector::LoadData()
{
	const FKPDataFile Data = KPData::Load();
	BaseTuning = Data.Tuning;
	KPVisuals::PlushSelfLight = BaseTuning.PlushSelfLight;          // T7b
	KPVisuals::TheatricalStuffGlow = BaseTuning.TheatricalStuffGlow;
	KPVisuals::HeroSelfLight = BaseTuning.HeroSelfLight;              // T7f
	KPVisuals::HeroHeight = BaseTuning.HeroHeight;
	Types = Data.Types;
	Nights = Data.Nights;
	Cards = Data.Cards;
	Modes = Data.Modes;
	Endless = Data.Endless;
	if (Modes.Num() == 0 || Modes[0].Id != TEXT("normal"))
	{
		Modes.Insert(FKPModeInfo(), 0); // Normal is always first
	}
	UiTexts = Data.Texts;
	WardrobeData = Data.Wardrobe; // T7d
	bWardrobeApplied = false;
	Audio = Data.Audio; // T7
	if (Audio.ResultsBeats.Num() == 0)
	{
		Audio.ResultsBeats = { 0.f, 1.f, 2.f, 3.f };
	}
	if (Save)
	{
		Save->FitNights(Nights.Num());
	}
}

void AKPDirector::LoadSave()
{
	Save = KPSave::LoadOrCreate();
	KPSettings::SetTheatrical(Save->Theatrical);
	if (!Save->CameraCutsDefaulted)
	{
		// T7e: camera cuts are Off by default now; a save from T7d had them On without anyone choosing it
		Save->CameraCutsDefaulted = true;
		Save->CameraCuts = false;
		KPSave::Write(Save);
	}
	if (!Save->CurseModesRenamed)
	{
		// T7d: the curse modes were renamed: Impossible (one curse a night) is now Demon, Godlike is now Impossible
		Save->CurseModesRenamed = true;
		auto Rename = [](const FString& Id) -> FString
		{
			return Id == TEXT("impossible") ? FString(TEXT("demon")) : Id == TEXT("godlike") ? FString(TEXT("impossible")) : Id;
		};
		for (FKPModeSave& Each : Save->ModeSaves)
		{
			Each.Id = Rename(Each.Id);
		}
		Save->SelectedMode = Rename(Save->SelectedMode);
		for (FString& Each : Save->ModesWon)
		{
			Each = Rename(Each);
		}
		TMap<FString, int32> Bests;
		for (const TPair<FString, int32>& Each : Save->ModeBest)
		{
			Bests.Add(Rename(Each.Key), Each.Value);
		}
		Save->ModeBest = Bests;
		KPSave::Write(Save);
	}
	if (!Save->ModesMigrated)
	{
		// T6c: a T6 save's week becomes Normal's week (finished if the week was complete), its Hardcore best
		// Hardcore's record
		Save->ModesMigrated = true;
		FKPModeSave& Normal = Save->FindModeSave(TEXT("normal"));
		Normal.Finished = Save->WeekComplete;
		Normal.Record = Save->WeekComplete ? Save->WeekScore() : 0;
		Normal.Night = Save->WeekComplete ? 0 : Save->CurrentNight;
		Normal.Active = !Save->WeekComplete && Save->CurrentNight > 0;
		if (Normal.Active)
		{
			Normal.NightScores = Save->NightBest;
		}
		const int32* HardcoreBest = Save->ModeBest.Find(TEXT("hardcore"));
		const int32 OldHardcore = FMath::Max(Save->HardcoreBest, HardcoreBest ? *HardcoreBest : 0);
		if (OldHardcore > 0 || Save->ModesWon.Contains(TEXT("hardcore")))
		{
			FKPModeSave& Hardcore = Save->FindModeSave(TEXT("hardcore"));
			Hardcore.Finished = Save->ModesWon.Contains(TEXT("hardcore"));
			Hardcore.Record = Hardcore.Finished ? OldHardcore : 0;
		}
		KPSave::Write(Save);
	}
	Result = FKPNightResult();
	Result.Night = -1;
}

void AKPDirector::SaveProgress()
{
	if (Save)
	{
		Save->Theatrical = KPSettings::IsTheatrical();
		if (bPractice && PracticeBackup)
		{
			// T8: Practice: only the settings reach the real save
			CopySettings(Save, PracticeBackup);
			KPSave::Write(PracticeBackup);
			return;
		}
		KPSave::Write(Save);
	}
}

void AKPDirector::SaveCutSetting()
{
	SaveProgress();
}

FString AKPDirector::Text(const TCHAR* Key, const TCHAR* Fallback) const
{
	const FString* Found = UiTexts.Find(Key);
	return Found ? *Found : FString(Fallback);
}

void AKPDirector::RecordNightEnd(bool bWon)
{
	if (!Save || Nights.Num() == 0)
	{
		return;
	}
	Save->FitNights(Nights.Num());
	Result = FKPNightResult();
	Result.bWon = bWon;
	Result.Night = NightIndex;
	Result.Score = Score;
	Result.Combo = BestCombo;
	Result.Cause = bWon ? FString() : LastHurtCause; // T7d
	BankPlayTime(); // T6d: this night's (round's) active play time joins the week's
	if (const FKPModeSave* Played = GetModeSave(ModeIndex))
	{
		Result.Time = Played->Time;
		Result.PrevBestTime = Played->BestTime;
		Result.BestTime = Played->BestTime;
	}
	if (IsEndless())
	{
		// T6c: an Extended cut run ends: the round reached; its records are the best score and the best round
		FKPModeSave& Run = Save->FindModeSave(CurrentMode().Id);
		Result.bEndless = true;
		Result.bRunOver = true;
		Result.Round = Wave;
		Result.PrevRecord = Run.Record;
		Result.PrevBestRound = Run.BestRound;
		Run.Record = FMath::Max(Run.Record, Score);
		Run.BestRound = FMath::Max(Run.BestRound, Wave);
		Run.Active = false;
		Result.Record = Run.Record;
		Result.BestRound = Run.BestRound;
		UiClock = 0.f;
		bResultsSoundPending = true; // T7: sfx_results, the stat lines on its beats
		SaveProgress();
		UE_LOG(LogKP, Log, TEXT("Run over (%s): round %d, score %d, best %d, best round %d"), *CurrentMode().Name, Wave, Score, Run.Record, Run.BestRound);
		return;
	}
	// Each night's best ever, in any mode (the HUD's "Best")
	Save->NightBest[NightIndex] = FMath::Max(Save->GetNightBest(NightIndex), Score);
	Save->NightBestCombo[NightIndex] = FMath::Max(Save->NightBestCombo[NightIndex], BestCombo);
	if (bWon)
	{
		Save->NightWon[NightIndex] = true;
	}
	Best = Save->NightBest[NightIndex];

	// T6c: the mode's own week. Its score is the sum of its nights (Normal: the best of each night's attempts);
	// its record is its best finished week
	const FKPModeInfo& Info = CurrentMode();
	FKPModeSave& Week = Save->FindModeSave(Info.Id);
	Week.NightScores.SetNumZeroed(Nights.Num());
	Week.NightScores[NightIndex] = Info.CarryHearts ? Score : FMath::Max(Week.NightScores[NightIndex], Score);
	const bool bLast = NightIndex >= Nights.Num() - 1;
	Result.PrevRecord = Week.Record;
	if (bWon && bLast)
	{
		Result.bWeekDone = true;
		Week.Record = FMath::Max(Week.Record, Week.WeekScore());
		// T6d: the fastest finished week
		Week.BestTime = Week.BestTime > 0.f ? FMath::Min(Week.BestTime, Week.Time) : Week.Time;
		Result.BestTime = Week.BestTime;
		Week.Finished = true; // (unlocks the next mode)
		Week.Active = false;
		Week.Night = 0;
		if (Info.Id == TEXT("normal"))
		{
			Save->WeekComplete = true;
		}
	}
	else if (bWon)
	{
		Week.Active = true;
		Week.Night = NightIndex + 1;
		Week.Hearts = Hearts; // (carried by the modes that carry them)
	}
	else if (Info.CarryHearts)
	{
		Result.bRunOver = true; // back to Monday
		Week.Active = false;
		Week.Night = 0;
	}
	else
	{
		Week.Active = true;
		Week.Night = NightIndex; // try the night again
	}
	Result.WeekScore = Week.WeekScore();
	Result.Record = Week.Record;
	UiClock = 0.f;
	bResultsSoundPending = Result.bWeekDone; // T7: the ending gets sfx_results (the stat lines on its beats)
	SaveProgress();
	UE_LOG(LogKP, Log, TEXT("Night over (%s, %s): score %d, week %d, record %d, hearts %d, combo %d, week time %.1f s, best time %.1f s"), bWon ? TEXT("won") : TEXT("lost"), *Info.Name, Score, Result.WeekScore, Result.Record, Hearts, BestCombo, Result.Time, Result.BestTime);
}

FString AKPDirector::NextNightLabel() const
{
	if (Result.bWon && Result.bWeekDone)
	{
		return Text(TEXT("backHome"), TEXT("Back to home"));
	}
	if (Result.bEndless)
	{
		return Text(TEXT("playAgainRun"), TEXT("Play again"));
	}
	if (Result.bRunOver)
	{
		return Text(TEXT("backToMonday"), TEXT("Back to Monday"));
	}
	const int32 Next = Result.bWon ? NightIndex + 1 : NightIndex;
	const FString NightName = Nights.IsValidIndex(Next) ? Nights[Next].Name : FString();
	const FString Label = Result.bWon ? Text(TEXT("nextNight"), TEXT("Next night: {night}")) : Text(TEXT("tryAgain"), TEXT("Try again: {night}"));
	return Label.Replace(TEXT("{night}"), *NightName);
}

int32 AKPDirector::CalendarScore(int32 Night) const
{
	const FKPModeSave* Week = GetModeSave(ModeIndex);
	return Week && Week->NightScores.IsValidIndex(Night) ? Week->NightScores[Night] : 0;
}

bool AKPDirector::CalendarWon(int32 Night) const
{
	// This week: every night before this one, and this one if it was won
	return Night < NightIndex || (Night == NightIndex && Result.bWon && Result.Night == NightIndex);
}

int32 AKPDirector::CalendarNext() const
{
	if (Result.bRunOver || Result.bWeekDone)
	{
		return -1;
	}
	return Result.bWon ? NightIndex + 1 : NightIndex;
}

FString AKPDirector::UiActionUnderCursor() const
{
	float MouseX = 0.f;
	float MouseY = 0.f;
	int32 ViewX = 0;
	int32 ViewY = 0;
	if (!Hud.IsValid() || !PC.IsValid() || !PC->GetMousePosition(MouseX, MouseY))
	{
		return FString();
	}
	PC->GetViewportSize(ViewX, ViewY);
	if (ViewX <= 0 || ViewY <= 0)
	{
		return FString();
	}
	return Hud->ActionAt(FVector2D(MouseX / ViewX, MouseY / ViewY));
}

void AKPDirector::ResetSave()
{
	// T7: the volumes and the sound test's picks survive a reset
	bPractice = false; // T8 (the practice copy is dropped along with the save)
	PracticeBackup = nullptr;
	const int32 MusicVolume = Save ? Save->MusicVolume : 8;
	const int32 SfxVolume = Save ? Save->SfxVolume : 8;
	const TMap<FString, FString> SoundPicks = Save ? Save->SoundPicks : TMap<FString, FString>();
	Save = KPSave::Reset();
	Save->Theatrical = KPSettings::IsTheatrical();
	Save->MusicVolume = MusicVolume;
	Save->SfxVolume = SfxVolume;
	Save->SoundPicks = SoundPicks;
	KPSave::Write(Save);
	Result = FKPNightResult();
	Result.Night = -1;
	GoToTitle(); // like a first launch: the cut choice, then the menu
}

void AKPDirector::TestResultsScreen(bool bEnding)
{
	// T7b: the ending (bEnding) or the endless run-over screen, right now, with made-up numbers; the save is untouched
	SetPaused(false);
	bAttract = false;
	TitlePage = 0;
	ClearPlushies();
	Path.Reset();
	PathLength = 0.f;
	Result = FKPNightResult();
	Result.Score = 48250;
	Result.Combo = 7;
	Result.Time = 1834.f;
	if (bEnding)
	{
		const int32 Normal = Modes.IndexOfByPredicate([](const FKPModeInfo& Each) { return !Each.Endless; });
		ModeIndex = FMath::Max(0, Normal);
		NightIndex = FMath::Max(0, Nights.Num() - 1);
		Result.bWon = true;
		Result.bWeekDone = true;
		Result.Night = NightIndex;
		Result.WeekScore = 312400;
		Result.PrevRecord = 298000;
		Result.Record = 312400;
		Result.PrevBestTime = 1950.f;
		Result.BestTime = 1834.f;
		Mode = EKPMode::Dawn;
	}
	else
	{
		const int32 EndlessIndex = Modes.IndexOfByPredicate([](const FKPModeInfo& Each) { return Each.Endless; });
		ModeIndex = FMath::Max(0, EndlessIndex);
		Result.bEndless = true;
		Result.bRunOver = true;
		Result.Round = 37;
		Result.PrevRecord = 51000;
		Result.Record = 51000;
		Result.PrevBestRound = 33;
		Result.BestRound = 37;
		Wave = 37;
		Mode = EKPMode::Over;
		PlaySound(TEXT("gameover"));
	}
	TsTarget = 1.f;
	UiClock = 0.f;
	GameOverTimer = 0.f;
	bResultsSoundPending = true;
	UE_LOG(LogKP, Log, TEXT("Test: the %s screen (made-up stats)"), bEnding ? TEXT("ending") : TEXT("run-over"));
}

void AKPDirector::UpdateResultsAuto(float RealDt)
{
	// Auto-cut test mode: a screenshot of the results screen (if any are left), then the next night
	if (UiClock >= 1.2f && UiClock - RealDt < 1.2f && AutoShotsLeft > 0)
	{
		--AutoShotsLeft;
		const FString File = FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/KP/Auto_%02d_%s.png"), ++AutoShotIndex, Mode == EKPMode::Dawn ? TEXT("dawn") : TEXT("lost"));
		FScreenshotRequest::RequestScreenshot(File, true, false);
	}
	if (UiClock >= 2.0f)
	{
		HandleUiAction(TEXT("next"));
	}
}

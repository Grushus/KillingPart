// Killing Part: the save game. Where in the week you are and each night's bests (T5b: the buttons,
// the workbench and the yard were removed). Saved at the end of every night (AKPDirector::SaveProgress).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KPSave.generated.h"

/** T6c: one game mode's week in progress and its record */
USTRUCT()
struct FKPModeSave
{
	GENERATED_BODY()

	UPROPERTY() FString Id;
	UPROPERTY() bool Active = false;        // a week to continue
	UPROPERTY() int32 Night = 0;            // the night to play next
	UPROPERTY() int32 Hearts = 3;           // hearts at the start of that night (modes that carry them)
	UPROPERTY() TArray<int32> NightScores;  // this week (Normal: the best of each night's attempts)
	UPROPERTY() TArray<int32> DarkWaves;    // night * 100 + wave (1-based): this week's dark waves
	UPROPERTY() int32 Record = 0;           // the best finished week (Extended cut: the best score)
	UPROPERTY() int32 BestRound = 0;        // T6c: Extended cut: the highest round reached
	UPROPERTY() bool Finished = false;      // a week was finished (unlocks the next mode)
	// T6d: active play time (real seconds while a night is played: not the countdown, card picks, pause,
	// introductions or results) and, for the endless modes, the run saved at the start of every round
	UPROPERTY() float Time = 0.f;           // this week (run) so far
	UPROPERTY() float BestTime = 0.f;       // the fastest finished week (0 = none yet)
	UPROPERTY() int32 Round = 0;            // endless: the round to start again on Continue
	UPROPERTY() int32 Score = 0;            // endless: the score at the start of that round
	UPROPERTY() TArray<FString> Cards;      // endless: held card ids at the start of that round (twice = level II)

	int32 WeekScore() const { int32 Sum = 0; for (const int32 Each : NightScores) { Sum += Each; } return Sum; }
};

UCLASS()
class UKPSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Version = 2;

	// The week: per night (0 = Monday) the best score and best combo ever, and whether it was won
	UPROPERTY() TArray<int32> NightBest;
	UPROPERTY() TArray<int32> NightBestCombo;
	UPROPERTY() TArray<bool> NightWon;
	UPROPERTY() int32 CurrentNight = 0;   // the night to play next (after a loss: the same one)
	UPROPERTY() int32 FurthestNight = 0;  // the furthest night reached
	UPROPERTY() bool WeekComplete = false;
	UPROPERTY() bool Theatrical = false;  // the chosen cut
	UPROPERTY() bool CutChosen = false;   // T6: the first launch asked which cut to watch
	UPROPERTY() bool TutorialCut = false; // T6: the tutorial's drawing prompts were shown (Monday's first wave)
	UPROPERTY() bool TutorialPick = false; // T6: and its card pick prompt
	UPROPERTY() TArray<FString> MetTypes; // T6: plushie types already introduced ("pincushion" for the pincushion)
	UPROPERTY() int32 HardcoreBest = 0;   // T6: the best Hardcore run (the sum of its nights' scores), kept apart from the week
	UPROPERTY() TMap<FString, int32> ModeBest; // T6b: each mode's best week (run) score; Normal's is WeekScore()
	UPROPERTY() TArray<FString> ModesWon;     // T6b: modes whose week was finished (unlocks)
	UPROPERTY() FString SelectedMode = TEXT("normal"); // T6b: the mode picked last (the title shows its best)
	UPROPERTY() TArray<FKPModeSave> ModeSaves; // T6c: every mode's own week and record
	UPROPERTY() bool ModesMigrated = false;   // T6c: the T6 week was moved into Normal's save
	// T7: the player's volumes (0-10, steps of 10%) and the sound test's picks (role -> option letter; kept by kp.ResetSave)
	UPROPERTY() int32 MusicVolume = 8; // T8: 80% on a first launch
	UPROPERTY() int32 SfxVolume = 8;
	UPROPERTY() TMap<FString, FString> SoundPicks;
	// T7d: camera cuts on 5+ and the boss's final split (pause and title), the wardrobe's picks (row -> swatch),
	// and the curse modes' rename (impossible -> demon, godlike -> impossible) done once
	UPROPERTY() bool CameraCuts = false;      // T7e: Off by default
	UPROPERTY() TMap<FString, int32> Wardrobe;
	UPROPERTY() bool CurseModesRenamed = false;
	UPROPERTY() bool CameraCutsDefaulted = false; // T7e: a T7d save (On by default then) is switched Off once

	/** The mode's save (made on first use) */
	FKPModeSave& FindModeSave(const FString& Id)
	{
		for (FKPModeSave& Each : ModeSaves)
		{
			if (Each.Id == Id)
			{
				return Each;
			}
		}
		FKPModeSave& Made = ModeSaves.AddDefaulted_GetRef();
		Made.Id = Id;
		return Made;
	}

	int32 GetNightBest(int32 Night) const { return NightBest.IsValidIndex(Night) ? NightBest[Night] : 0; }
	int32 GetNightBestCombo(int32 Night) const { return NightBestCombo.IsValidIndex(Night) ? NightBestCombo[Night] : 0; }
	bool IsNightWon(int32 Night) const { return NightWon.IsValidIndex(Night) && NightWon[Night]; }

	/** Makes the per-night arrays NumNights long */
	void FitNights(int32 NumNights);

	/** The sum of every night's best score */
	int32 WeekScore() const;
};

namespace KPSave
{
	/** The save on disk, or a fresh one */
	UKPSaveGame* LoadOrCreate();
	bool Write(UKPSaveGame* Save);
	/** Deletes the save on disk and returns a fresh one */
	UKPSaveGame* Reset();
}

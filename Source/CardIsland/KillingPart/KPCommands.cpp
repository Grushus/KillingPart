// Killing Part: console commands for testing. Open the console while playing (the key left of 1)
// and type for example: kp.Wave 5

#include "KillingPart/KPDirector.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING // T8: no kp commands in the release build
namespace KPCommands
{
	static AKPDirector* FindDirector(UWorld* World)
	{
		if (World)
		{
			TActorIterator<AKPDirector> It(World);
			if (It)
			{
				return *It;
			}
		}
		UE_LOG(LogKP, Warning, TEXT("Killing Part commands only work while the game is running. Press Play first."));
		return nullptr;
	}

	static FAutoConsoleCommandWithWorldAndArgs GodCommand(
		TEXT("kp.God"),
		TEXT("Toggles god mode: plushies still pop on touch but cost no hearts. kp.God 1 or kp.God 0 to set it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				const bool bOn = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !Director->IsGodMode();
				Director->SetGodMode(bOn);
				UE_LOG(LogKP, Log, TEXT("God mode %s"), bOn ? TEXT("on") : TEXT("off"));
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs SoundTestCommand(
		TEXT("kp.SoundTest"),
		TEXT("T7: opens or closes the sound test panel (also F8): every sound role with its options; click an option to cycle and hear it. Picks are saved locally. kp.SoundTest 1 or 0 to set it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				const bool bOn = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !Director->IsSoundTestOpen();
				if (bOn != Director->IsSoundTestOpen())
				{
					Director->ToggleSoundTest();
				}
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs CloseUpCommand(
		TEXT("kp.CloseUp"),
		TEXT("T7d test: a close camera in front of her (kp.CloseUp 0 to go back)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				Director->SetCloseUpTest(Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0);
			}
		}));

	// T7b test commands
	static FAutoConsoleCommandWithWorldAndArgs UnlockAllCommand(
		TEXT("kp.UnlockAll"),
		TEXT("T7b test: unlocks every mode (until kp.LockAll or the game closes; nothing is saved)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				Director->SetUnlockAll(true);
				UE_LOG(LogKP, Log, TEXT("Every mode unlocked (kp.LockAll puts it back)"));
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs LockAllCommand(
		TEXT("kp.LockAll"),
		TEXT("T7b test: the modes' locks back to what the save has earned."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				Director->SetUnlockAll(false);
				UE_LOG(LogKP, Log, TEXT("Mode locks back to the save's"));
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs TestEndingCommand(
		TEXT("kp.TestEnding"),
		TEXT("T7b test: shows the week's ending screen now, with made-up stats (sfx_results and the stat lines on its beats). Nothing is saved."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				Director->TestResultsScreen(true);
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs TestRunOverCommand(
		TEXT("kp.TestRunOver"),
		TEXT("T7b test: shows the Extended cut's run-over screen now, with made-up stats (sfx_gameover, then sfx_results and the stat lines). Nothing is saved."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				Director->TestResultsScreen(false);
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs WaveCommand(
		TEXT("kp.Wave"),
		TEXT("Clears the mat and starts wave N of the current night (cards and hearts stay). Example: kp.Wave 3"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				Director->JumpToWave(Args.Num() > 0 ? FMath::Max(1, FCString::Atoi(*Args[0])) : 1);
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs AutoCutCommand(
		TEXT("kp.AutoCut"),
		TEXT("Test mode: she cuts through the nearest plushies by herself. kp.AutoCut 1 or kp.AutoCut 0 to set it. Best with kp.God 1."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				const bool bOn = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !Director->IsAutoCut();
				Director->SetAutoCut(bOn);
				Director->SetAutoBossOnly(Args.Num() > 0 && FCString::Atoi(*Args[0]) == 2);
				UE_LOG(LogKP, Log, TEXT("Auto cut %s"), bOn ? TEXT("on") : TEXT("off"));
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs CutCommand(
		TEXT("kp.Cut"),
		TEXT("Which version to watch: kp.Cut theatrical (white stuffing, no blood) or kp.Cut director (blood). Without a word it switches."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			bool bTheatrical = !KPSettings::IsTheatrical();
			if (Args.Num() > 0)
			{
				if (Args[0].StartsWith(TEXT("t"), ESearchCase::IgnoreCase))
				{
					bTheatrical = true;
				}
				else if (Args[0].StartsWith(TEXT("d"), ESearchCase::IgnoreCase))
				{
					bTheatrical = false;
				}
				else
				{
					UE_LOG(LogKP, Warning, TEXT("kp.Cut takes theatrical or director"));
					return;
				}
			}
			KPSettings::SetTheatrical(bTheatrical);
			UE_LOG(LogKP, Log, TEXT("%s"), bTheatrical ? TEXT("Theatrical cut: white stuffing, no blood") : TEXT("Director's cut: blood"));
			// The setting is global, so this also works before the game runs; a running game updates at once (and saves it)
			if (World)
			{
				for (TActorIterator<AKPDirector> It(World); It; ++It)
				{
					It->ApplyCut(true);
					It->SaveCutSetting();
				}
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs ResetSaveCommand(
		TEXT("kp.ResetSave"),
		TEXT("Deletes the save (night bests, the current night, week complete) and starts Monday fresh"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				Director->ResetSave();
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs UiCommand(
		TEXT("kp.Ui"),
		TEXT("Clicks the results screen's button, for testing: kp.Ui next (Next night / Try again / Play the week again)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				Director->ClickUi(Args.Num() > 0 ? Args[0] : FString(TEXT("next")));
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs NightCommand(
		TEXT("kp.Night"),
		TEXT("Starts night N (1 = Monday ... 7 = Sunday), fresh: hearts, score and cards reset. Example: kp.Night 4"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				Director->StartNight(Args.Num() > 0 ? FMath::Max(1, FCString::Atoi(*Args[0])) - 1 : 0);
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs CardCommand(
		TEXT("kp.Card"),
		TEXT("Takes a card for the rest of the night, by id or name: kp.Card longchalk, kp.Card long chalk, kp.Card thimble"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				const FString Name = FString::Join(Args, TEXT(""));
				if (!Director->TakeCard(Name))
				{
					FString Known;
					for (const FKPCard& Card : Director->GetCards())
					{
						Known += Card.Id + TEXT(" ");
					}
					UE_LOG(LogKP, Warning, TEXT("No card called %s. Cards: %s"), *Name, *Known);
				}
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs SpawnCommand(
		TEXT("kp.Spawn"),
		TEXT("Spawns plushies of one type at the mat's edge: kp.Spawn bunny 3. Types are in KillingPart.json (teddy, bunny, bigbear, cub, golden, patchwork, hen, chick); kp.Spawn pin places pincushions."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				const FString TypeName = Args.Num() > 0 ? Args[0] : FString(TEXT("teddy"));
				const int32 Count = Args.Num() > 1 ? FMath::Clamp(FCString::Atoi(*Args[1]), 1, 40) : 1;
				if (!Director->SpawnByName(TypeName, Count))
				{
					UE_LOG(LogKP, Warning, TEXT("No plushie type called %s in KillingPart.json"), *TypeName);
				}
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs PauseCommand(
		TEXT("kp.Pause"),
		TEXT("Pauses or resumes, like Esc (in the editor Esc stops Play, so use P or this). kp.Pause 1 or kp.Pause 0 to set it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				Director->SetPaused(Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !Director->IsGamePaused());
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs RestartCommand(
		TEXT("kp.Restart"),
		TEXT("Restarts the current night and re-reads Content/Data/KillingPart.json"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AKPDirector* Director = FindDirector(World))
			{
				Director->ResetRun();
			}
		}));
}
#endif

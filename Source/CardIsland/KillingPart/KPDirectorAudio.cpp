// Killing Part: sound (T7). Every number, the option picked per role and the switches are in
// KillingPart.json ("audio"). Roles (keys of audio.sounds): snip, squishDirector, squishTheatrical, whoosh,
// chalk, squeak, hurt, killingpart, waveclear, cardhover, click, tick, lightsout, pin, results, musicMenu,
// musicNight. The sound test (kp.SoundTest, F8) cycles a role's options; its picks are saved locally
// (UKPSaveGame::SoundPicks) and win over the JSON's until Simon writes them in as defaults.
//
// Music: two looping components, the menu's and the night's, crossfaded. While drawing (slow motion) the
// night music is low-passed and ducked; on a dark wave it stays low-passed the whole wave (audio.darkMusic
// "menu" plays the menu music on dark waves instead). Everything here runs on real (undilated) time.
//
// T7b: the night music's options play in turn (a, then b, then a...), not looping; a new wave while b plays
// crossfades back to a. Losing plays sfx_gameover while the night music fades out, then the menu music comes in.
// New roles: enrage (the boss's enrage tell), gameover, peep (a chick arrives), intro (a new plushie's
// introduction), goldenpin (a golden pincushion cut).

#include "KillingPart/KPDirector.h"
#include "KillingPart/KPPlayerController.h"
#include "Components/AudioComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"

namespace KPAudioConst
{
	constexpr float NoFilterHz = 20000.f;
	// The sound test's rows, in order
	const TCHAR* const TestRoles[] = {
		TEXT("snip"), TEXT("squishDirector"), TEXT("squishTheatrical"), TEXT("whoosh"), TEXT("chalk"), TEXT("squeak"),
		TEXT("hurt"), TEXT("killingpart"), TEXT("waveclear"), TEXT("cardhover"), TEXT("click"), TEXT("tick"),
		TEXT("lightsout"), TEXT("pin"), TEXT("results"), TEXT("enrage"), TEXT("gameover"), TEXT("peep"), TEXT("intro"),
		TEXT("goldenpin"), TEXT("blink"), TEXT("musicMenu"), TEXT("musicNight") };
}

UAudioComponent* AKPDirector::MakeAudioComponent(const TCHAR* Name)
{
	UAudioComponent* Component = NewObject<UAudioComponent>(this, Name);
	Component->bAutoActivate = false;
	Component->bAutoDestroy = false;
	Component->bAllowSpatialization = false;
	Component->bIsUISound = true; // keeps playing while the game is paused
	Component->SetupAttachment(GetRootComponent());
	Component->RegisterComponent();
	return Component;
}

void AKPDirector::SetupAudio()
{
	MenuMusic = MakeAudioComponent(TEXT("MenuMusic"));
	NightMusic = MakeAudioComponent(TEXT("NightMusic"));
	NightMusicB = MakeAudioComponent(TEXT("NightMusicB"));
	ChalkSound = MakeAudioComponent(TEXT("ChalkLoop"));
}

USoundWave* AKPDirector::SoundFor(const FString& RoleName)
{
	const FKPSoundRole* Found = Audio.Sounds.Find(RoleName);
	if (!Found || Found->Options.Num() == 0)
	{
		return nullptr;
	}
	return SoundOption(RoleName, SoundPickIndex(RoleName));
}

USoundWave* AKPDirector::SoundOption(const FString& RoleName, int32 Index)
{
	const FKPSoundRole* Found = Audio.Sounds.Find(RoleName);
	if (!Found || Found->Options.Num() == 0)
	{
		return nullptr;
	}
	const FString& Name = Found->Options[((Index % Found->Options.Num()) + Found->Options.Num()) % Found->Options.Num()];
	if (const TObjectPtr<USoundWave>* Cached = SoundCache.Find(Name))
	{
		return Cached->Get();
	}
	const FString AssetPath = FString::Printf(TEXT("/Game/KillingPart/Audio/%s.%s"), *Name, *Name);
	USoundWave* Clip = LoadObject<USoundWave>(nullptr, *AssetPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Clip)
	{
		UE_LOG(LogKP, Warning, TEXT("Sound %s (role %s) is missing"), *AssetPath, *RoleName);
	}
	SoundCache.Add(Name, Clip); // (a missing one is remembered as null, so it's only looked for once)
	return Clip;
}

int32 AKPDirector::SoundPickIndex(const FString& RoleName) const
{
	const FKPSoundRole* Found = Audio.Sounds.Find(RoleName);
	if (!Found || Found->Options.Num() == 0)
	{
		return 0;
	}
	FString Letter = Found->Pick;
	if (Save)
	{
		if (const FString* Local = Save->SoundPicks.Find(RoleName))
		{
			Letter = *Local;
		}
	}
	const int32 Index = Letter.IsEmpty() ? 0 : FChar::ToLower(Letter[0]) - TEXT('a');
	return FMath::Clamp(Index, 0, Found->Options.Num() - 1);
}

void AKPDirector::PlaySound(const FString& RoleName, float VolumeMul, float Delay, bool bTest)
{
	if (Delay > 0.f)
	{
		PendingSounds.Add({ RoleName, Delay, VolumeMul, bTest });
		return;
	}
	const FKPSoundRole* Found = Audio.Sounds.Find(RoleName);
	if (!Found)
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	if (!bTest && Found->MinGap > 0.f)
	{
		const double* Last = LastPlayed.Find(RoleName);
		if (Last && Now - *Last < Found->MinGap)
		{
			return;
		}
	}
	LastPlayed.Add(RoleName, Now);
	float Volume = Found->Volume * Audio.SfxVolume * GetSfxVolume() * 0.1f * VolumeMul;
	if (bAttract && !Found->Ui && !bTest)
	{
		Volume *= Audio.TitleSfx; // the title's mat plays itself: quiet (or silent) by default
	}
	if (Volume <= 0.001f)
	{
		return;
	}
	USoundWave* Clip = SoundFor(RoleName);
	if (!Clip)
	{
		return;
	}
	const float Pitch = Found->Pitch * FMath::FRandRange(1.f - Found->PitchRandom, 1.f + Found->PitchRandom);
	UGameplayStatics::PlaySound2D(this, Clip, Volume, Pitch, 0.f, nullptr, nullptr, Found->Ui || bTest);
}

void AKPDirector::PlaySnipSounds(int32 Count, bool bKillingPart)
{
	if (Count <= 0)
	{
		return;
	}
	// One snip, then a few squishes layered just after it (each softer), not one per plushie at full volume
	PlaySound(TEXT("snip"));
	const FString Squish = KPSettings::IsTheatrical() ? TEXT("squishTheatrical") : TEXT("squishDirector");
	const int32 Squishes = FMath::Min(Count, FMath::Max(1, Audio.SquishesPerSnip));
	float Share = 1.f;
	for (int32 Index = 0; Index < Squishes; ++Index)
	{
		PlaySound(Squish, Share, Index * Audio.SquishSpread);
		Share *= Audio.SquishFalloff;
	}
	// A death squeak or two, not on every plushie
	int32 Squeaks = 0;
	for (int32 Index = 0; Index < Count && Squeaks < Audio.SqueaksPerSnip; ++Index)
	{
		if (FMath::FRand() < Audio.SqueakChance)
		{
			PlaySound(TEXT("squeak"), 1.f, FMath::FRandRange(Audio.SqueakDelayMin, Audio.SqueakDelayMax));
			++Squeaks;
		}
	}
	if (bKillingPart)
	{
		PlaySound(TEXT("killingpart"));
	}
}

float AKPDirector::ResultsSoundAt() const
{
	return Result.bEndless ? Audio.RunOverResultsAt : Audio.EndingResultsAt;
}

float AKPDirector::ResultsLineTime(int32 Line) const
{
	const TArray<float>& Beats = Audio.ResultsBeats;
	float Beat = Line;
	if (Beats.IsValidIndex(Line))
	{
		Beat = Beats[Line];
	}
	else if (Beats.Num() > 0)
	{
		Beat = Beats.Last() + (Line - Beats.Num() + 1); // past the last beat: one a second
	}
	return ResultsSoundAt() + Beat;
}

bool AKPDirector::HandleVolumeAction(const FString& Action)
{
	// vol:music:1, vol:sfx:-1
	if (!Action.StartsWith(TEXT("vol:")) || !Save)
	{
		return false;
	}
	TArray<FString> Parts;
	Action.ParseIntoArray(Parts, TEXT(":"));
	if (Parts.Num() < 3)
	{
		return true;
	}
	const int32 Step = FCString::Atoi(*Parts[2]);
	int32& Value = Parts[1] == TEXT("music") ? Save->MusicVolume : Save->SfxVolume;
	Value = FMath::Clamp(Value + Step, 0, 10);
	SaveProgress();
	PlaySound(TEXT("click")); // (heard at the new volume)
	UE_LOG(LogKP, Log, TEXT("Volume: music %d0%%, sounds %d0%%"), Save->MusicVolume, Save->SfxVolume);
	return true;
}

void AKPDirector::ToggleSoundTest()
{
	bSoundTest = !bSoundTest;
	if (!bSoundTest)
	{
		MusicPreview.Reset();
		ChalkTestTimer = 0.f;
	}
	UE_LOG(LogKP, Log, TEXT("Sound test %s"), bSoundTest ? TEXT("open") : TEXT("closed"));
}

void AKPDirector::SoundTestRoles(TArray<FString>& Out) const
{
	Out.Reset();
	for (const TCHAR* RoleName : KPAudioConst::TestRoles)
	{
		Out.Add(RoleName);
	}
	// Any other role the JSON has, after them
	for (const TPair<FString, FKPSoundRole>& Each : Audio.Sounds)
	{
		Out.AddUnique(Each.Key);
	}
}

void AKPDirector::HandleSoundTestAction(const FString& Action)
{
	if (Action == TEXT("st:close"))
	{
		if (bSoundTest)
		{
			ToggleSoundTest();
		}
		return;
	}
	if (Action == TEXT("st:reset"))
	{
		// Back to the JSON's picks (after they've been written in as defaults)
		if (Save)
		{
			Save->SoundPicks.Reset();
			SaveProgress();
		}
		MusicPreview.Reset();
		UE_LOG(LogKP, Log, TEXT("Sound test: local picks cleared, using KillingPart.json's"));
		return;
	}
	const bool bCycle = Action.StartsWith(TEXT("st:cycle:"));
	const bool bPlay = Action.StartsWith(TEXT("st:play:"));
	if (!bCycle && !bPlay)
	{
		return;
	}
	const FString RoleName = Action.RightChop(bCycle ? 9 : 8);
	const FKPSoundRole* Found = Audio.Sounds.Find(RoleName);
	if (!Found)
	{
		return;
	}
	const bool bMusic = RoleName.StartsWith(TEXT("music"));
	if (bCycle && Found->Options.Num() > 0 && Save)
	{
		const int32 Next = (SoundPickIndex(RoleName) + 1) % Found->Options.Num();
		Save->SoundPicks.Add(RoleName, FString::Chr(static_cast<TCHAR>(TEXT('a') + Next)));
		SaveProgress();
		UE_LOG(LogKP, Log, TEXT("Sound test: %s = %s (%s)"), *RoleName, *FString::Chr(static_cast<TCHAR>(TEXT('a') + Next)), *Found->Options[Next]);
	}
	if (bMusic)
	{
		// Music: play hears it (play again stops the preview); cycling hears the new track
		MusicPreview = (bPlay && MusicPreview == RoleName) ? FString() : RoleName;
		if (RoleName == TEXT("musicNight"))
		{
			// T7b: start the night tracks over from the picked one
			NightMusic->Stop();
			NightMusicB->Stop();
			bNightTrackStarted[0] = bNightTrackStarted[1] = false;
			NightTrack = 0;
			NightTrackStep = 0;
			NightTrackLevel[0] = 1.f;
			NightTrackLevel[1] = 0.f;
			return;
		}
		if (UAudioComponent* Component = MenuMusic.Get())
		{
			USoundWave* Clip = SoundFor(RoleName);
			if (Component->Sound != Clip)
			{
				Component->SetSound(Clip);
			}
		}
		return;
	}
	if (RoleName == TEXT("chalk"))
	{
		ChalkTestTimer = 2.5f; // a looping sound: a moment of it
		if (ChalkSound)
		{
			ChalkSound->SetSound(SoundFor(RoleName));
		}
		return;
	}
	PlaySound(RoleName, 1.f, 0.f, true);
}

void AKPDirector::UpdateAudio(float RealDt)
{
	if (!MenuMusic || !NightMusic || !NightMusicB || !ChalkSound)
	{
		return;
	}
	// F8: the sound test (in the editor F8 also ejects from the player; kp.SoundTest works everywhere)
#if !UE_BUILD_SHIPPING
	if (PC.IsValid() && PC->WasInputKeyJustPressed(EKeys::F8))
	{
		ToggleSoundTest();
	}
#endif

	// Delayed sounds (the layered squishes, squeaks)
	for (int32 Index = 0; Index < PendingSounds.Num(); ++Index)
	{
		PendingSounds[Index].Delay -= RealDt;
		if (PendingSounds[Index].Delay <= 0.f)
		{
			const FKPPendingSound Due = PendingSounds[Index];
			PendingSounds.RemoveAt(Index--);
			PlaySound(Due.RoleName, Due.Volume, 0.f, Due.bTest);
		}
	}

	// The 3-2-1: a tick on each number, the snip on "go"
	const int32 Number = (Countdown > 0.f && !bAttract) ? FMath::CeilToInt(Countdown) : 0;
	if (Number != LastCountdownNumber)
	{
		if (Number > 0)
		{
			PlaySound(TEXT("tick"));
		}
		else if (LastCountdownNumber == 1 && Mode == EKPMode::Play)
		{
			PlaySound(TEXT("snip"));
		}
		LastCountdownNumber = Number;
	}

	// The ending and the run-over screen: sfx_results, with the stat lines on its beats (the HUD reads ResultsLineTime)
	if (bResultsSoundPending && (Mode == EKPMode::Dawn || Mode == EKPMode::Over) && UiClock >= ResultsSoundAt())
	{
		bResultsSoundPending = false;
		PlaySound(TEXT("results"));
	}

	// Music: the menu's on the title and the results screens, the night's otherwise
	const bool bDark = bLightsOut && !bAttract;
	bool bMenu = bAttract || Mode == EKPMode::Over || Mode == EKPMode::Dawn || (bDark && Audio.DarkMusic.Equals(TEXT("menu"), ESearchCase::IgnoreCase));
	if (!MusicPreview.IsEmpty())
	{
		bMenu = MusicPreview == TEXT("musicMenu");
	}
	// T7b: lost: sfx_gameover (played by Hurt) while the night music fades out, then the menu music
	const bool bLost = Mode == EKPMode::Over && !bAttract && MusicPreview.IsEmpty();
	GameOverTimer = bLost ? GameOverTimer + RealDt : 0.f;
	const bool bQuiet = bLost && GameOverTimer < Audio.GameOverQuiet;
	const float Fade = RealDt / FMath::Max(0.05f, Audio.Crossfade);
	const float NightFade = bLost ? RealDt / FMath::Max(0.05f, Audio.GameOverFade) : Fade;
	MenuMusicLevel = FMath::Clamp(MenuMusicLevel + ((bMenu && !bQuiet) ? Fade : -Fade), 0.f, 1.f);
	NightMusicLevel = FMath::Clamp(NightMusicLevel + (bMenu ? -NightFade : NightFade), 0.f, 1.f);
	// Drawing: muffled and quieter; a dark wave: muffled the whole wave
	const bool bDrawing = Mode == EKPMode::Draw && !bAttract && MusicPreview.IsEmpty();
	const bool bDarkMuffle = bDark && MusicPreview.IsEmpty();
	const float WantHz = bDrawing ? Audio.DrawLowPassHz : bDarkMuffle ? Audio.DarkLowPassHz : KPAudioConst::NoFilterHz;
	const float WantDuck = bDrawing ? Audio.DrawMusicVolume : bDarkMuffle ? Audio.DarkMusicVolume : 1.f;
	const float Blend = FMath::Min(1.f, RealDt / FMath::Max(0.01f, Audio.LowPassFade) * 3.f);
	// (the cutoff glides in log space, so it sweeps evenly to the ear)
	LowPassHz = FMath::Exp(FMath::Lerp(FMath::Loge(FMath::Max(20.f, LowPassHz)), FMath::Loge(FMath::Max(20.f, WantHz)), Blend));
	MusicDuck = FMath::Lerp(MusicDuck, WantDuck, Blend);
	const float MusicBase = Audio.MusicVolume * GetMusicVolume() * 0.1f * MusicDuck;
	auto DriveMusic = [&](UAudioComponent* Component, const TCHAR* RoleName, float Level)
	{
		USoundWave* Clip = SoundFor(RoleName);
		if (Component->Sound != Clip)
		{
			Component->SetSound(Clip);
		}
		const FKPSoundRole* Found = Audio.Sounds.Find(RoleName);
		const float Volume = Level * MusicBase * (Found ? Found->Volume : 1.f);
		if (Volume > 0.001f && Clip)
		{
			if (!Component->IsPlaying())
			{
				Component->Play();
			}
			Component->SetVolumeMultiplier(Volume);
			const bool bFilter = LowPassHz < KPAudioConst::NoFilterHz * 0.9f;
			// (the setters only reach the playing sound; the properties carry it into the next Play)
			Component->bEnableLowPassFilter = bFilter;
			Component->LowPassFilterFrequency = LowPassHz;
			Component->SetLowPassFilterEnabled(bFilter);
			Component->SetLowPassFilterFrequency(LowPassHz);
		}
		else if (Component->IsPlaying() && Level <= 0.f)
		{
			Component->Stop();
		}
		else
		{
			Component->SetVolumeMultiplier(0.f);
		}
	};
	DriveMusic(MenuMusic, TEXT("musicMenu"), MenuMusicLevel);
	UpdateNightMusic(RealDt, NightMusicLevel, MusicBase);

	// The chalk loop while drawing (quietly), or for a moment in the sound test
	ChalkTestTimer = FMath::Max(0.f, ChalkTestTimer - RealDt);
	const bool bChalk = (Audio.ChalkLoop && Mode == EKPMode::Draw && !bAttract && !bPaused) || ChalkTestTimer > 0.f;
	const float ChalkStep = RealDt / FMath::Max(0.01f, Audio.ChalkFade);
	ChalkLevel = FMath::Clamp(ChalkLevel + (bChalk ? ChalkStep : -ChalkStep), 0.f, 1.f);
	const FKPSoundRole* ChalkRole = Audio.Sounds.Find(TEXT("chalk"));
	const float ChalkVolume = ChalkLevel * (ChalkRole ? ChalkRole->Volume : 0.f) * Audio.SfxVolume * GetSfxVolume() * 0.1f;
	if (ChalkVolume > 0.001f)
	{
		if (!ChalkSound->IsPlaying())
		{
			ChalkSound->SetSound(SoundFor(TEXT("chalk")));
			ChalkSound->SetPitchMultiplier(ChalkRole ? ChalkRole->Pitch * FMath::FRandRange(1.f - ChalkRole->PitchRandom, 1.f + ChalkRole->PitchRandom) : 1.f);
			ChalkSound->Play();
		}
		ChalkSound->SetVolumeMultiplier(ChalkVolume);
	}
	else if (ChalkSound->IsPlaying())
	{
		ChalkSound->Stop();
	}
}

void AKPDirector::UpdateNightMusic(float RealDt, float Level, float MusicBase)
{
	// T7b: musicNight's options in turn (from the pick: a, b, a...), each played once through; a new wave
	// while a later track plays crossfades back to the first. T7c: only music_night_a is left, and it loops
	// (the asset's looping flag), so it simply plays on
	const FKPSoundRole* NightRole = Audio.Sounds.Find(TEXT("musicNight"));
	UAudioComponent* Comps[2] = { NightMusic.Get(), NightMusicB.Get() };
	if (!NightRole || NightRole->Options.Num() == 0)
	{
		return;
	}
	auto StopTrack = [&](int32 Index)
	{
		if (Comps[Index]->IsPlaying())
		{
			Comps[Index]->Stop();
		}
		bNightTrackStarted[Index] = false;
	};
	if (Level <= 0.f)
	{
		// Silent: back to the first track for next time
		StopTrack(0);
		StopTrack(1);
		NightTrack = 0;
		NightTrackStep = 0;
		NightTrackLevel[0] = 1.f;
		NightTrackLevel[1] = 0.f;
		MusicWave = Wave;
		return;
	}
	const int32 Pick = SoundPickIndex(TEXT("musicNight"));
	if (Wave != MusicWave)
	{
		MusicWave = Wave;
		if (NightTrackStep != 0 && !bAttract)
		{
			// A new wave: the first track again, crossfading from the later one
			NightTrack = 1 - NightTrack;
			NightTrackStep = 0;
			StopTrack(NightTrack);
			UE_LOG(LogKP, Log, TEXT("Night music: new wave, back to the first track"));
		}
	}
	else if (bNightTrackStarted[NightTrack] && !Comps[NightTrack]->IsPlaying())
	{
		// The track ended: the next one follows straight on (no crossfade, nothing left to fade)
		bNightTrackStarted[NightTrack] = false;
		NightTrack = 1 - NightTrack;
		++NightTrackStep;
		StopTrack(NightTrack);
		NightTrackLevel[NightTrack] = 1.f;
		NightTrackLevel[1 - NightTrack] = 0.f;
		UE_LOG(LogKP, Log, TEXT("Night music: track ended, next is option %d"), (Pick + NightTrackStep) % NightRole->Options.Num());
	}
	const float Step = RealDt / FMath::Max(0.05f, Audio.WaveCrossfade);
	const bool bFilter = LowPassHz < KPAudioConst::NoFilterHz * 0.9f;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const bool bCurrent = Index == NightTrack;
		NightTrackLevel[Index] = FMath::Clamp(NightTrackLevel[Index] + (bCurrent ? Step : -Step), 0.f, 1.f);
		UAudioComponent* Component = Comps[Index];
		const float Volume = Level * MusicBase * NightRole->Volume * NightTrackLevel[Index];
		if (bCurrent && !bNightTrackStarted[Index] && Volume > 0.001f)
		{
			NightTrackOption[Index] = (Pick + NightTrackStep) % NightRole->Options.Num();
			Component->SetSound(SoundOption(TEXT("musicNight"), NightTrackOption[Index]));
			Component->bEnableLowPassFilter = bFilter;
			Component->LowPassFilterFrequency = LowPassHz;
			Component->SetVolumeMultiplier(Volume);
			Component->Play();
			bNightTrackStarted[Index] = true;
		}
		if (!bCurrent && NightTrackLevel[Index] <= 0.f)
		{
			StopTrack(Index);
			continue;
		}
		if (Component->IsPlaying())
		{
			Component->SetVolumeMultiplier(FMath::Max(Volume, 0.f));
			Component->bEnableLowPassFilter = bFilter;
			Component->LowPassFilterFrequency = LowPassHz;
			Component->SetLowPassFilterEnabled(bFilter);
			Component->SetLowPassFilterFrequency(LowPassHz);
		}
	}
}

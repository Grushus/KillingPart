// Killing Part: the game. One actor runs the core loop from DESIGN.md (draw in slow motion,
// dash at real speed, the beat, the snip), the waves, the hearts and score, and the juice.
// Plushies have no brains; this moves them. AKPGameMode spawns it.
//
// Time: the world runs on global time dilation (Ts). Everything that must stay at real speed
// (the dash, the beat, timers, shake) uses the undilated frame time; the game-time step is Dt * Ts.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KillingPart/KPTypes.h"
#include "KillingPart/KPSave.h"
#include "KPDirector.generated.h"

class AKPHeroine;
class AKPPlushie;
class AKPPincushion;
class AKPPlayerController;
class ACameraActor;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class UMaterialInterface;
class USpotLightComponent;
class UPointLightComponent;
class ULightComponent;
class SKPHud;
class SWeakWidget;
class AKPPortrait;
class UTextureRenderTarget2D;
class USoundWave;
class UAudioComponent;

UENUM()
enum class EKPMode : uint8
{
	Play,
	Draw,
	Dash,
	Snip,
	Over,   // lost all hearts: the results screen ("Hugged to bits", Try again)
	Pick,   // between waves: pick 1 of 3 cards
	Dawn,   // the night's last wave is cleared: the results screen (Next night)
	Intro   // T6: before a wave, meet a new plushie (a portrait over the blurred mat; click to go on)
};

/** How a night ended, for the results screen */
struct FKPNightResult
{
	bool bWon = false;
	int32 Night = 0;
	int32 Score = 0;
	int32 WeekScore = 0;   // this week so far (the mode's save)
	int32 Combo = 0;       // this night's best combo
	bool bWeekDone = false; // this win finished the week (Sunday)
	bool bRunOver = false;  // T6c: lost in a mode that goes back to Monday (or an Extended cut run ended)
	bool bEndless = false;  // T6c: an Extended cut run: Round reached, records are best score and best round
	int32 Round = 0;
	int32 PrevBestRound = 0;
	int32 BestRound = 0;
	int32 PrevRecord = 0;   // T6c: the mode's record before this week
	int32 Record = 0;
	float Time = 0.f;       // T6d: the week's (run's) active play time so far
	float PrevBestTime = 0.f; // T6d: the mode's best (fastest finished) week before this one (0 = none)
	float BestTime = 0.f;
	FString Cause;          // T7d: what took the last heart (plushie, pincushion, impostor, boss)
};

/** Where the card pick's cards are, as fractions of the viewport (shared by the HUD and the click test) */
namespace KPCardLayout
{
	inline FBox2D CardRect(int32 Index, int32 Count)
	{
		const double Width = 0.2;
		const double Height = 0.42;
		const double Gap = 0.03;
		const double Total = Count * Width + (Count - 1) * Gap;
		const double Left = 0.5 - Total * 0.5 + Index * (Width + Gap);
		const double Top = 0.3;
		return FBox2D(FVector2D(Left, Top), FVector2D(Left + Width, Top + Height));
	}
}

/** Text that floats up and fades: combo names, "Hugged", "Wave 3" */
struct FKPFloatText
{
	FVector World = FVector::ZeroVector;
	bool bOnScreen = false;      // true: Screen is a 0-1 position on the screen instead of World
	FVector2D Screen = FVector2D::ZeroVector;
	FString Text;
	FString Sub;
	float Life = 1.f;
	float MaxLife = 1.f;
	float Size = 15.f;           // in the toy's pixels; the HUD scales it
	FLinearColor Color = FLinearColor::White;
};

UCLASS()
class AKPDirector : public AActor
{
	GENERATED_BODY()

public:
	AKPDirector();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Restarts the current night (re-reads KillingPart.json) */
	void ResetRun();

	/** Starts night NightIndex (0 = Monday), wrapping after the last; re-reads KillingPart.json */
	void StartNight(int32 NightIndex);

	/** Takes a card by id or name (kp.Card, or a click on the card pick). False if there is no such card. */
	bool TakeCard(const FString& IdOrName);

	// Testing (console commands in KPCommands.cpp)
	void SetGodMode(bool bOn) { bGod = bOn; }
	bool IsGodMode() const { return bGod; }
	void JumpToWave(int32 WaveNumber);
	/** Makes the game match KPSettings::IsTheatrical(): cap colour, and no blood left on screen in the Theatrical cut */
	void ApplyCut(bool bAnnounce);
	/** Test mode: she cuts through the nearest plushies by herself (kp.AutoCut, or -KPAutoCut on the command line) */
	void SetAutoCut(bool bOn) { bAutoCut = bOn; AutoCutTimer = 0.5f; }
	/** T6b test: the auto-cutter only goes for the boss while it lives (kp.AutoCut 2) */
	void SetAutoBossOnly(bool bOn) { bAutoBossOnly = bOn; }
	bool IsAutoCut() const { return bAutoCut; }
	/** Spawns Count plushies of the named type at the mat's edge (kp.Spawn); false if there is no such type */
	bool SpawnByName(const FString& TypeName, int32 Count);

	// The save and the results screen (T5, cut down in T5b; KPDirectorWeek.cpp)
	/** T7b test: kp.UnlockAll (every mode open) / kp.LockAll (back to what the save earned) */
	void SetUnlockAll(bool bOn) { bUnlockAll = bOn; }
	/** T7b test: kp.TestEnding / kp.TestRunOver: the week's ending or the endless run-over screen, with made-up stats */
	void TestResultsScreen(bool bEnding);
	/** kp.ResetSave: deletes the save and starts Monday fresh */
	void ResetSave();
	/** kp.Cut: remembers the chosen cut in the save */
	void SaveCutSetting();
	/** kp.Ui next: does what clicking the results screen's button does */
	void ClickUi(const FString& Action) { HandleUiAction(Action); }

	// The title screen (T6; KPDirectorMenu.cpp)
	/** Back to the title: the menu over the mat, plushies wandering in and her cutting them by herself */
	void GoToTitle();
	bool IsTitle() const { return bAttract; }
	/** 0 = the menu, 1 = the first launch's cut choice, 3 = the modes (Play) */
	int32 GetTitlePage() const { return TitlePage; }
	/** Shaders or pipeline states are still being prepared (the first launch of a build) */
	bool IsPreparing() const;
	float GetRealClock() const { return RealClock; }

	// Game modes (T6b, a ladder with a save each since T6c; KPDirectorMenu.cpp)
	const TArray<FKPModeInfo>& GetModes() const { return Modes; }
	/** The mode being played (0 = Normal) */
	int32 GetModeIndex() const { return ModeIndex; }
	const FKPModeInfo& CurrentMode() const;
	bool IsModeUnlocked(int32 Index) const;
	/** A mode's record (its best finished week) */
	int32 GetModeBest(int32 Index) const;
	/** The mode's week in progress, or null */
	const FKPModeSave* GetModeSave(int32 Index) const;
	/** "Continue: Wednesday, 10,050" (endless: "Continue: Round 23, 41,200") for mode Index, or "" when it has nothing to continue */
	FString ModeContinueLabel(int32 Index) const;
	/** T6d: the modes screen's two cards: side 0 = the week modes, 1 = the endless modes. The mode index shown on a side (-1 = none) */
	int32 GetShownMode(int32 Side) const;
	/** The mode indices on a side, in order */
	void ModesOnSide(int32 Side, TArray<int32>& Out) const;
	/** T6c: an Extended cut run is on */
	bool IsEndless() const;
	/** T8: a practice night (any night, Normal rules, nothing saved) */
	bool IsPractice() const { return bPractice; }
	/** The 3-2-1 before a night (real seconds left; 0 = none) */
	float GetCountdown() const { return Countdown; }
	/** The week calendar's numbers: this week's scores in the current mode */
	int32 CalendarScore(int32 Night) const;
	bool CalendarWon(int32 Night) const;
	int32 CalendarNext() const;

	// Pause (T6): Esc (or P) stops the world; its menu is painted by the HUD
	void SetPaused(bool bOn);
	bool IsGamePaused() const { return bPaused; }
	/** How much the mat behind the HUD is blurred (pause, a new plushie's introduction) */
	float GetBlur() const { return BlurAmount; }

	// The tutorial (T6): Monday's first wave, first time only
	/** The prompt at the bottom of the screen, or "" */
	FString GetTutorialText() const;
	float GetTutorialAlpha() const { return FMath::Clamp(TutorialFade, 0.f, 1.f); }

	// Meet a new plushie (T6): for the HUD
	FString GetIntroName() const { return IntroName; }
	FString GetIntroLine() const { return IntroLine; }
	bool IsIntroHazard() const { return IntroKey == TEXT("pincushion"); }
	bool IsIntroBoss() const { return bIntroBoss; }
	UTextureRenderTarget2D* GetPortraitTexture() const;

	// The boss (T6): for the HUD; -1 when there is none (T6c: hits left)
	int32 GetBossCutsLeft() const;
	FString GetBossName() const;

	// For the HUD: the results screen
	const UKPSaveGame* GetSave() const { return Save; }
	const TArray<FKPNight>& GetNights() const { return Nights; }
	const FKPNightResult& GetResult() const { return Result; }
	float GetUiClock() const { return UiClock; }
	/** A text from KillingPart.json ("texts"), or Fallback */
	FString Text(const TCHAR* Key, const TCHAR* Fallback) const;
	/** What the results screen's button says ("Next night: Tuesday", "Try again: Tuesday", "Play the week again") */
	FString NextNightLabel() const;

	// Audio (T7; KPDirectorAudio.cpp)
	/** The player's volumes, 0-10 (steps of 10%) */
	int32 GetMusicVolume() const { return Save ? FMath::Clamp(Save->MusicVolume, 0, 10) : 10; }
	int32 GetSfxVolume() const { return Save ? FMath::Clamp(Save->SfxVolume, 0, 10) : 10; }
	/** The sound test panel (kp.SoundTest, F8): for trying the sound options; never shown to players otherwise */
	void ToggleSoundTest();
	bool IsSoundTestOpen() const { return bSoundTest; }
	/** The roles listed in the sound test, in order */
	void SoundTestRoles(TArray<FString>& Out) const;
	const FKPSoundRole* FindSoundRole(const FString& RoleName) const { return Audio.Sounds.Find(RoleName); }
	/** The option a role uses (0 = a): the sound test's local pick, else the JSON's */
	int32 SoundPickIndex(const FString& RoleName) const;
	/** The music the sound test is previewing ("" for none) */
	const FString& GetMusicPreview() const { return MusicPreview; }
	/** The ending and the run-over screen: when (UiClock) stat line Line appears, on sfx_results' beats */
	float ResultsLineTime(int32 Line) const;
	/** kp.SoundTest's actions ("st:cycle:<role>", "st:play:<role>", "st:close") */
	void HandleSoundTestAction(const FString& Action);

	// T7d: camera cuts (a low, close camera on her for a Killing part and the boss's final split)
	bool AreCameraCutsOn() const { return !Save || Save->CameraCuts; }
	/** The cinematic camera is on (the HUD draws letterbox bars) */
	bool IsCineActive() const { return CineMode == 1 || CineMode == 2; }
	/** T7d: the title's wardrobe page is open */
	bool IsWardrobe() const { return bAttract && TitlePage == 4; }
	/** T7d: the wardrobe's rows (0 dress, 1 apron, 2 hair and bow, 3 shears, 4 dash trail): name, swatches, pick */
	static constexpr int32 NumWardrobeRows = 5;
	/** T7f: the Tripo model has one baked texture, so the wardrobe shows only the rows from here on (shears, trail);
	 * the dress, apron and hair rows keep their code and saved picks */
	static constexpr int32 FirstShownWardrobeRow = 3;
	FString WardrobeRowName(int32 Row) const;
	void WardrobeSwatches(int32 Row, TArray<FLinearColor>& Out) const;
	int32 WardrobePick(int32 Row) const;
	/** Test: a close camera on her (kp.CloseUp, -KPCloseUp) */
	void SetCloseUpTest(bool bOn) { bCloseUpTest = bOn; }

	// For the HUD
	const FKPTuning& GetTuning() const { return Tuning; }
	EKPMode GetMode() const { return Mode; }
	int32 GetHearts() const { return Hearts; }
	int32 GetScore() const { return Score; }
	int32 GetBest() const { return Best; }
	int32 GetWave() const { return Wave; }
	int32 GetNumWaves() const { return CurrentNight().Waves.Num(); }
	const FKPNight& CurrentNight() const;
	FString NextNightName() const;
	bool IsLastNight() const { return NightIndex >= Nights.Num() - 1; }
	const TArray<int32>& GetOffer() const { return Offer; }
	const TArray<FKPCard>& GetCards() const { return Cards; }
	const TArray<int32>& GetHeldCards() const { return Held; }
	/** A card's number from KillingPart.json ("values"), or Default */
	float CardValue(const FString& Id, const TCHAR* Key, float Default) const;
	/** A card's boon or loose thread with its "{key}" numbers filled in ({heart} stays, the HUD draws it) */
	FString CardText(int32 CardIndex, bool bBoon) const;
	/** T7e: what the held cards and curses add up to, as short lines: the boons and the loose threads (the HUD's bottom row) */
	void GetActiveEffects(TArray<FString>& Boons, TArray<FString>& Threads) const;
	/** How many times card CardIndex is held (its level: 0 = not held, max 3) */
	int32 GetCardLevel(int32 CardIndex) const { int32 N = 0; for (const int32 Each : Held) { N += Each == CardIndex ? 1 : 0; } return N; }
	int32 GetHoveredCard() const { return HoveredCard; }
	float GetTimeScale() const { return Ts; }
	float GetChalkLeft01() const;
	float GetFlash() const { return Flash; }
	FLinearColor GetFlashColor() const { return FlashColor; }
	bool ShouldShowHint() const { return bHint && Mode == EKPMode::Play; }
	int32 GetPreviewCount() const { return PreviewCount; }
	bool GetLineEnd(FVector& OutEnd) const;
	const TArray<FKPFloatText>& GetTexts() const { return Texts; }
	AKPPlayerController* GetPlayerController() const { return PC.Get(); }

private:
	struct FParticle
	{
		FVector Pos;
		FVector Vel;
		float Life;
		float Size;
		int32 Kind;
	};

	struct FTrailDot
	{
		FVector Pos;
		float Life;
	};

	enum EParticleKind : int32 { Blood = 0, DarkBlood = 1, Fluff = 2, NumKinds = 3 };

	/** Seam ripper: a pin flying from a snipped plushie to a neighbour it cuts */
	struct FFlyingPin
	{
		FVector From;
		FVector To;
		float Age = 0.f;       // real seconds; negative = not launched yet
		float Duration = 0.2f;
		float Arc = 60.f;
		bool bLast = false;    // the last pin to a target: its arrival slices it
		int32 ColorIndex = 0;  // its head colour (one of the pincushion's)
		TWeakObjectPtr<AKPPlushie> Target;
	};

	// Setup
	bool EnsureWorldRefs();
	void SetupVisuals();
	void BuildGrid();
	void CreateHud();
	void RemoveHud();
	UInstancedStaticMeshComponent* MakeIsm(const TCHAR* Name);

	// The loop
	void UpdateInput();
	void AddPathPoint(FVector Point);
	void StartDash();
	void UpdateAutoCut(float RealDt);
	void UpdateWaves(float GameDt, float RealDt);
	int32 PickType() const;
	/** T7d: the type has shown up this week (any night so far; Extended cut: the round's mix) */
	bool TypeSeenThisWeek(const FString& Name) const;
	/** T7d: a random variant whose two types have shown up this week, or INDEX_NONE */
	int32 PickVariant() const;
	int32 FindType(const FString& Name) const;
	AKPPlushie* SpawnPlushie(int32 TypeIndex);
	AKPPlushie* SpawnPlushieAt(int32 TypeIndex, const FVector& Pos, float Radius, const FLinearColor& Color, float Speed);
	float WaveSpeed() const;
	void SpawnPin();
	// T7d: the impostor
	void EnsureImpostorRoll(int32 WaveNumber);
	bool SpawnImpostor();
	int32 ImpostorRollWave = -1;  // the wave the roll below is for
	bool bImpostorRoll = false;   // that wave brings an impostor
	float ImpostorTimer = -1.f;   // >= 0: game seconds until this wave's impostor appears
	/** T7c: the blinking pincushion: from Thursday (Extended cut: round blinkFromRound), blinkChance of new pincushions */
	bool RollBlinkPin() const;
	bool CanBlinkPinsNow(int32 WaveNumber) const;
	/** T7c: a free spot on the mat for a pincushion (not near her or another one), false if none found */
	bool FindPinSpot(FVector& Out, const AKPPincushion* Ignore) const;
	void UpdatePins(float GameDt);
	void UpdateSplits(float GameDt);
	bool HasPendingSplits() const;
	void UpdatePlushies(float GameDt, float RealDt);
	void LayEggs(float GameDt);
	bool SpawnPinInPath();
	void FadeOutPins();
	void UpdateDash(float RealDt);
	void Sweep(const FVector& From, const FVector& To, float Yaw);
	void DoSnip();
	void ProcessSliceQueue();
	/** Loses a heart. Why: the word over her ("" for none); Cause (T7d): what the results screen blames (plushie, pincushion, impostor, boss) */
	void Hurt(const FString& Why, const TCHAR* Cause = TEXT("plushie"));
	FString LastHurtCause;
	void ClearPlushies();
	void UpdatePieces(float GameDt);
	const FKPWave& CurrentWave() const;
	void ApplyCards();
	void OpenPick();
	int32 CardUnderCursor() const;
	void SetLightsOut(bool bOn);
	void UpdateLamp();
	void SeamRip();
	void UpdateFlyingPins(float RealDt);
	void ExtraInchTargets(const FVector& End, TArray<AKPPlushie*>& Out) const;
	/** The Extra inch cone at the line's end: its tip, forward direction, half angle (radians) and length; false if there is none */
	bool ExtraInchCone(FVector& OutTip, FVector& OutDir, float& OutHalfAngle, float& OutLength) const;
	/** Magnet: the first pincushion along the line (in drawing order) that it will cut harmlessly, or null */
	AKPPincushion* MagnetTarget() const;
	void ApplyExtraInch();
	void UpdateChalkLight(float RealDt);
	/** Lights out: how lit a spot on the mat is (1 inside her lamp or near the chalk light, 0 in the dark) */
	float LightAt(const FVector& Pos) const;

	// The save and the results screen (KPDirectorWeek.cpp)
	void LoadData();
	void LoadSave();
	void SaveProgress();
	/** A night ended (dawn or lost): bests go in the save, and the screen's numbers in Result */
	void RecordNightEnd(bool bWon);
	void HandleUiAction(const FString& Action);
	/** The action under the mouse on the HUD's results screen ("" for none) */
	FString UiActionUnderCursor() const;
	void UpdateResultsAuto(float RealDt);

	// Audio (T7; KPDirectorAudio.cpp)
	void SetupAudio();
	/** Music (crossfade, the muffle while drawing and on dark waves), the chalk loop, delayed sounds, the 3-2-1's ticks */
	void UpdateAudio(float RealDt);
	/** Plays a role's picked option (small random pitch). Delay in real seconds. bTest: the sound test (ignores titleSfx and minGap). */
	void PlaySound(const FString& RoleName, float VolumeMul = 1.f, float Delay = 0.f, bool bTest = false);
	/** A snip that cut Count: one snip, a few squishes, a squeak or two, killingpart for a big combo */
	void PlaySnipSounds(int32 Count, bool bKillingPart);
	USoundWave* SoundFor(const FString& RoleName);
	/** T7b: a role's option Index (wrapping), cached like SoundFor */
	USoundWave* SoundOption(const FString& RoleName, int32 Index);
	UAudioComponent* MakeAudioComponent(const TCHAR* Name);
	/** vol:music:1 / vol:sfx:-1 (the title and pause menus' arrows); true if it was one */
	bool HandleVolumeAction(const FString& Action);
	float ResultsSoundAt() const;
	FKPAudioData Audio;
	UPROPERTY() TMap<FString, TObjectPtr<USoundWave>> SoundCache;
	UPROPERTY() TObjectPtr<UAudioComponent> MenuMusic;
	UPROPERTY() TObjectPtr<UAudioComponent> NightMusic;
	UPROPERTY() TObjectPtr<UAudioComponent> NightMusicB;   // T7b: the night tracks play in turn (a, b, a...); two components so they can crossfade
	float NightTrackLevel[2] = { 1.f, 0.f };
	bool bNightTrackStarted[2] = { false, false };
	int32 NightTrack = 0;           // the component playing the current track (0 = NightMusic, 1 = NightMusicB)
	int32 NightTrackOption[2] = { 0, 0 }; // the musicNight option each component holds
	int32 NightTrackStep = 0;       // how many tracks have followed the first (0 = the first, music_night_a)
	int32 MusicWave = 0;            // the wave the night music last saw (a new wave goes back to the first track)
	float GameOverTimer = 0.f;      // T7b: real seconds since losing (the night music fades, sfx_gameover plays, then the menu music)
	UPROPERTY() TObjectPtr<UAudioComponent> ChalkSound;
	float MenuMusicLevel = 0.f;
	float NightMusicLevel = 0.f;
	float MusicDuck = 1.f;          // drawing / dark wave: the music's volume share
	float LowPassHz = 20000.f;      // the music's low-pass cutoff (20 kHz = off)
	float ChalkLevel = 0.f;
	float ChalkTestTimer = 0.f;     // the sound test plays the chalk loop for a moment
	struct FKPPendingSound
	{
		FString RoleName;
		float Delay = 0.f;
		float Volume = 1.f;
		bool bTest = false;
	};
	TArray<FKPPendingSound> PendingSounds;
	TMap<FString, double> LastPlayed;
	int32 LastCountdownNumber = 0;
	/** T7b: the night music's turn-taking tracks */
	void UpdateNightMusic(float RealDt, float Level, float MusicBase);
	bool bResultsSoundPending = false;
	bool bWaveClearSounded = true;
	bool bSoundTest = false;
	FString MusicPreview;

	// The title screen (KPDirectorMenu.cpp)
	void UpdateAttract(float GameDt);
	void UpdateTitleAuto(float RealDt);
	/** Leaves the title (or a menu) for night NightIndex */
	void PlayNight(int32 InNightIndex);
	void QuitToDesktop();
	/** Esc / P and the pause menu's clicks (runs while the world is paused) */
	void UpdatePauseInput();
	void UpdateTutorial(float RealDt);

	// Meet a new plushie (KPDirectorMenu.cpp)
	/** The types wave WaveNumber can bring that have an introduction and haven't been met ("pincushion" too) */
	void FindNewcomers(int32 WaveNumber, TArray<FString>& Out) const;
	/** Shows the next introduction in IntroQueue; false when there are none left */
	bool ShowNextIntro();
	void FinishIntro();

	// The boss (KPDirectorBoss.cpp)
	void SpawnBoss(const FString& TypeName, int32 Count = 1, int32 HitsOverride = 0);
	bool AnyBossAlive() const;
	TArray<TWeakObjectPtr<AKPPlushie>> Bosses;   // T6c: the bosses of this wave (BossPlushie is the first)
	FKPEndless Endless;
	FKPNight EndlessNight;                        // T6c: the run as one long night, its waves made round by round
	void EnsureEndlessRound(int32 RoundNumber);
	void UpdateBoss(float GameDt, float RealDt);
	/** A snip that went through the boss: a chunk off it, or the final split. Returns the points it's worth. */
	int32 CutBoss(AKPPlushie* Boss);
	/** The final split: slow motion held, everything left on the mat falls apart, "Killing part", then dawn */
	void StartFinale(AKPPlushie* Boss);

	// Juice
	void Burst(const FVector& At, int32 Count, const FVector& Normal, bool bBlood);
	void Spray(const FVector& At, const FVector& Normal, bool bBlood);
	void Stain(const FVector& At, float Size, bool bSplat);
	void StainSplat(const FVector& At, float Radius, const FVector& Normal);
	void UpdateFx(float RealDt, float GameDt);
	void AddWorldText(const FVector& At, const FString& Text, const FString& Sub, float Life, float Size, const FLinearColor& Color);
	void AddScreenText(const FVector2D& At01, const FString& Text, float Life, float Size, const FLinearColor& Color);

	// Showing it
	void UpdatePreview();
	void UpdateLineVisuals();
	void UpdateHero(float RealDt);
	void UpdateCamera();
	static void SetInstances(UInstancedStaticMeshComponent* Ism, const TArray<FTransform>& Transforms);

	FKPTuning BaseTuning;   // as loaded
	FKPTuning Tuning;       // with the night's rule and the cards applied (ApplyCards)
	TArray<FKPNight> Nights;
	TArray<FKPCard> Cards;
	TArray<FKPModeInfo> Modes;
	TMap<FString, FString> UiTexts;
	FKPWardrobe WardrobeData;     // T7d
	/** T7d: her colours and the dash trail's, from the save's wardrobe picks */
	void ApplyWardrobe();
	void OpenWardrobe();
	FLinearColor WardrobeColor(int32 Row, bool bSecond = false) const;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> TrailMaterial;
	bool bWardrobeApplied = false;

	// The save and the results screen
	UPROPERTY() TObjectPtr<UKPSaveGame> Save;
	// T8: Practice. The real save waits in PracticeBackup; the game plays on a copy, and only the settings
	// (cut, camera cuts, volumes, sound picks, wardrobe) go back into the real save when it is written
	UPROPERTY() TObjectPtr<UKPSaveGame> PracticeBackup;
	bool bPractice = false;
	void StartPractice(int32 Night);
	void EndPractice();
	void CopySettings(const UKPSaveGame* From, UKPSaveGame* To) const;
	FKPNightResult Result;
	int32 BestCombo = 0;         // this night
	float UiClock = 0.f;         // real seconds since the results screen opened

	// The title screen
	bool bAttract = false;       // the title is up: the mat plays itself behind the menu
	int32 TitlePage = 0;
	float AttractSpawnTimer = 0.f;
	bool bTitleTest = false;     // -KPTitle with -KPAutoCut: a screenshot of the title, then Continue

	// The mode being played (T6c: every week is a mode with its own save)
	int32 ModeIndex = 0;
	FKPModeSave* ModeSaveMut();
	/** A fresh week in mode Index (rolls its dark waves), then Monday */
	void NewGame(int32 Index);
	/** The mode's saved week, at its saved night */
	void ContinueGame(int32 Index);
	void RollDarkWaves(FKPModeSave& Week) const;
	bool IsDarkWave(int32 Night, int32 WaveNumber) const;
	float Countdown = 0.f;
	int32 AliveCount() const;
	// T6d
	int32 ShownMode[2] = { -1, -1 };   // the modes screen: the mode each card shows (-1 = not chosen yet)
	void StepShownMode(int32 Side, int32 Step);
	double PlayTime = 0.0;             // this attempt's active play time (real seconds), banked into the mode's save
	/** Adds PlayTime to the mode's save (night end, restart, save & quit) */
	void BankPlayTime();
	bool bCountdownNext = false;       // the next StartNight shows the 3-2-1 (only after Continue)
	bool bCountdownAfterIntro = false; // ...after the night's first introduction
	/** Endless: the run's state at the start of round Wave (Continue starts that round again) */
	void SaveEndlessRound();
	/** Endless: back to the saved round (hearts, cards, score); false without one */
	bool ResumeEndlessRun();

	// Pause
	bool bPaused = false;
	float BlurAmount = 0.f;

	// The tutorial: 0 off, 1 "hold and drag", 2 "let go", 3 "combo" (for a few seconds), 4 the first card pick
	int32 TutorialStep = 0;
	float TutorialTimer = 0.f;
	float TutorialFade = 0.f;
	FString TutorialShown;      // the prompt on screen (it fades out before the next one fades in)
	FString TutorialWanted() const;

	// Meet a new plushie
	TArray<FString> IntroQueue;   // type names (or "pincushion") still to introduce before this wave
	FString IntroKey;
	FString IntroName;
	FString IntroLine;
	bool bIntroBoss = false;
	UPROPERTY() TObjectPtr<AKPPortrait> Portrait;

	// The boss
	TWeakObjectPtr<AKPPlushie> BossPlushie;
	int32 BossCutsTaken = 0;
	float EscortTimer = 0.f;
	float FinaleTimer = 0.f;     // real seconds of the final split's held slow motion
	bool bBossDown = false;      // split for good: nothing gets up again (big bears cut in the finale stay down)
	struct FBossChunk
	{
		TWeakObjectPtr<AKPPlushie> Owner;
		int32 Index = 0;         // the boss's chunk
		FVector At = FVector::ZeroVector; // where it was cut off (if the chunk is gone)
		float Timer = 0.5f;      // game seconds until it gets up
	};
	TArray<FBossChunk> BossChunks;   // T6b: chunks waiting to get up as small teddies
	int32 BossHitsLeft = 0;          // T6c
	float ChickTimer = 0.f;          // T6c: game seconds to the next flock of chicks
	int32 PendingSpawns = 0;         // T6c: plushies (or pincushions) owed by hits, waiting under the alive cap
	int32 PendingChicks = 0;
	bool bBossEnraged = false;
	float BossTellTimer = 0.f;   // real seconds left of the enrage tell (shake, glinting eye)

	// T6b: a night whose first wave brings an introduction shows its banner after it (name and rule name only)
	bool bNightBannerPending = false;
	void ShowNightBanner(bool bWithText);
	bool bCursePickAtStart = false;  // T7c: the night opened on a curse pick (Impossible); its banner follows the pick

	// World
	TWeakObjectPtr<AKPPlayerController> PC;
	TWeakObjectPtr<AKPHeroine> Hero;
	TWeakObjectPtr<ACameraActor> Camera;
	FTransform CameraBase;
	// T7d: the second camera (camera cuts, the wardrobe, kp.CloseUp)
	TWeakObjectPtr<ACameraActor> CineCam;
	int32 CineMode = 0;          // 0 the game camera, 1 a Killing part's dash and beat, 2 the boss's final split
	FVector CineDir = FVector::ForwardVector;
	FVector CineSide = FVector::RightVector;
	FVector CineLoc = FVector::ZeroVector;
	FVector CineLook = FVector::ZeroVector;
	bool bCineSnap = true;
	bool bCloseUpTest = false;
	TWeakObjectPtr<AKPPlushie> CineBoss;
	/** Plushies (not chicks) the line goes through now (the camera cut's test at the dash) */
	int32 CountOnPath() const;
	void StartCine(int32 InMode);
	UPROPERTY() TArray<TObjectPtr<AKPPlushie>> Plushies;
	UPROPERTY() TArray<TObjectPtr<AKPPlushie>> CutPlushies;   // sliced, oldest first; each carries two halves
	UPROPERTY() TArray<TObjectPtr<AKPPlushie>> SliceQueue;    // snipped, waiting their turn to be sliced (a few per frame)
	UPROPERTY() TArray<TObjectPtr<AKPPincushion>> Pins;
	TArray<FKPPlushieType> Types;

	// Run state
	EKPMode Mode = EKPMode::Play;
	int32 Hearts = 3;
	int32 Score = 0;
	int32 Best = 0;
	int32 Wave = 0;          // 1-based within the night; 0 before the first
	int32 NightIndex = 0;
	bool bBreakDone = true;  // the cleared wave's card pick (or dawn) has happened
	float BreakTimer = 0.f;

	// Cards
	TArray<int32> Held;      // indices into Cards
	TArray<int32> Offer;     // the cards on the table during a pick
	int32 HoveredCard = -1;
	float SpeedMul = 1.f;
	float ScoreMul = 1.f;
	int32 ExtraPerWave = 0;
	int32 ExtraPins = 0;
	bool bDoubleStitch = false;
	bool bSeamRipper = false;
	bool bReturning = false; // Double stitch: on the way back
	bool bExtraInch = false;
	int32 ExtraInchLevel = 0;
	int32 SeamLevel = 0;
	int32 MagnetLevel = 0;   // Magnet: this many pincushions per wave are cut harmlessly
	int32 MagnetLeft = 0;    // left this wave
	float WaveCountMul = 1.f; // Thimble: more plushies per wave
	float AutoPickTimer = 0.f;
	TArray<FFlyingPin> FlyingPins;
	TArray<FTrailDot> PinTrail;   // the Seam ripper pins' short bright trails
	UPROPERTY() TArray<TObjectPtr<AKPPlushie>> PinTargets;   // Seam ripper: marked, waiting for their pins

	// Lights out
	bool bLightsOut = false;
	TArray<TPair<TWeakObjectPtr<ULightComponent>, float>> DimmedLights;
	UPROPERTY() TObjectPtr<USpotLightComponent> Lamp;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> ChalkLightComps;
	TArray<FVector> LitLine;      // where the chalk light is (the line being drawn, then the last cut)
	float ChalkLightLevel = 0.f;  // 1 while drawing, fades to 0 after the cut
	float RealClock = 0.f;

	// Waves
	TArray<int32> SpawnList;      // this wave's spawns still to come: a type index, or -1 for a pick from the mix
	int32 PathPinsLeft = 0;       // pincushions still to place in her way this wave
	float PathPinTimer = 0.f;
	int32 Queue = 0;
	float SpawnTimer = 0.f;
	float WaveTimer = 0.f;
	float Cooldown = 0.f;
	float Invulnerable = 0.f;
	float SnipTimer = 0.f;
	bool bHint = true;
	bool bGod = false;
	bool bWasHeld = false;

	// Test mode (auto cut, and screenshots after its snips)
	bool bAutoCut = false;
	bool bAutoBossOnly = false;
	float AutoCutTimer = 0.f;
	int32 AutoShotsLeft = 0;
	int32 AutoShotIndex = 0;
	float AutoShotTimer = 0.f;
	bool bBeatShots = false;
	TArray<float> ShotTimes;   // T7d test (-KPShotAt=3,6): screenshots after these real seconds, whatever is on screen   // T7d test (-KPBeatShots): the auto screenshots are taken during the beat, not after the snip

	// Time
	float Ts = 1.f;
	float TsTarget = 1.f;

	// Her
	FVector HeroPos = FVector::ZeroVector;
	float HeroYaw = 90.f;
	float ShearsAfterSnip = 0.f;   // T7d: real seconds the closed shears stay visible after the snip

	// The cut
	TArray<FVector> Path;
	float PathLength = 0.f;
	int32 DashIndex = 0;
	float DashS = 0.f;
	float DashTime = 0.f;
	UPROPERTY() TArray<TObjectPtr<AKPPlushie>> Marked;
	int32 PreviewCount = 0;
	TArray<bool> PreviewFlags;

	// Juice
	float Shake = 0.f;
	float HitStopTimer = 0.f;    // T7b: real seconds the world stays frozen after a snip
	int32 GoldenPinsCut = 0;     // T7b: golden pincushions this line went through (scored at the snip)
	int32 WaveSpawned = 0;       // T7e: plushies spawned this wave (for the log line at the wave's end)
	int32 WaveGolden = 0;        // T7e: golden ones among them (golden, golden big bear and its cubs, the gold impostor)
	int32 WaveGoldenRolled = 0;  // T7e: golden plushies picked by the golden roll itself (not split cubs)
	TArray<FVector> GoldenPinSpots;
	bool bUnlockAll = false;     // T7b test: kp.UnlockAll
	float Flash = 0.f;
	FLinearColor FlashColor = FLinearColor::White;
	TArray<FParticle> Particles;
	TArray<FTrailDot> Trail;
	TArray<FKPFloatText> Texts;
	int32 StainCursor = 0;
	int32 SplatCursor = 0;

	// Instanced visuals
	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> GridIsm;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> GridMajorIsm;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> ChalkIsm;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> RingIsm;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> WarnIsm;    // red rings: pincushions the line crosses
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> TrailIsm;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> FlyPinIsm;      // Seam ripper pins: shafts
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> FlyPinHeadIsm;  // and their heads
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> PinTrailIsm;    // and their trails
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> FlyPinHeadIsms; // heads, one per colour
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> BandIsm;        // Pinking shears: the line's cut width, while drawing
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SafeIsm;        // Magnet: the ring on a pincushion that will be cut harmlessly
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> BlinkRingIsm;   // T7d: a blinking pincushion's warning ring in the dark (purple)
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> BlinkLights;      // T7d: and the small light that pulses on its spot
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> StainIsm;   // small: where drops land
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SplatIsm;   // big: under each snip, kept all night
	UPROPERTY() TObjectPtr<UNiagaraSystem> BurstSystem;               // NS_KPBurst: a spray of streaks, coloured by User.Color
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> ParticleIsms;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ChalkMaterial;
	UPROPERTY() TObjectPtr<UMaterialInterface> CapBase;   // M_KPCap; each cut plushie makes its own instance

	// HUD
	TSharedPtr<SKPHud> Hud;
	TSharedPtr<SWeakWidget> HudHost;
	TSharedPtr<SWidget> HudRoot;   // the blur and the HUD in an overlay (SWeakWidget holds it only weakly)
};

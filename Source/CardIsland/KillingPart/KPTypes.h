// Killing Part: shared types, tuning numbers and small helpers.
// The tuning defaults match DESIGN.md (the toy's numbers x3.5). Content/Data/KillingPart.json
// overrides them; it is re-read on every restart, so numbers can be changed while playing.

#pragma once

#include "CoreMinimal.h"
#include "KPTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogKP, Log, All);

class UMaterialInstanceDynamic;
class UStaticMesh;

/** All the numbers that shape the feel. World units: X is up the screen, Y is right, the mat top is Z = 0. */
USTRUCT()
struct FKPTuning
{
	GENERATED_BODY()

	// Arena (the cutting mat), centred on the origin
	UPROPERTY() float ArenaHalfX = 720.f;
	UPROPERTY() float ArenaHalfY = 1200.f;

	// Her
	UPROPERTY() float HeroRadius = 40.f;
	UPROPERTY() int32 Hearts = 3;
	UPROPERTY() float Invulnerable = 1.1f;
	UPROPERTY() float TouchSlack = 7.f;

	// Plushies
	UPROPERTY() float PlushieRadiusMin = 42.f;
	UPROPERTY() float PlushieRadiusMax = 56.f;
	UPROPERTY() float PlushieSpeedBase = 63.f;
	UPROPERTY() float PlushieSpeedPerWave = 11.f;
	UPROPERTY() float PlushieSpeedRandom = 32.f;

	// Waves (endless for now)
	UPROPERTY() int32 WaveSizeBase = 3;
	UPROPERTY() int32 WaveSizePerWave = 2;
	UPROPERTY() float SpawnInterval = 0.45f;
	UPROPERTY() float SpawnPad = 77.f;
	UPROPERTY() float FirstWaveDelay = 0.8f;
	UPROPERTY() float WaveDelay = 1.2f;

	// Drawing the cut
	UPROPERTY() float Chalk = 1500.f;
	UPROPERTY() float PointSpacing = 17.f;
	UPROPERTY() float MinLine = 60.f;
	UPROPERTY() float EdgeMargin = 28.f;

	// Time
	UPROPERTY() float DrawTimeScale = 0.07f;
	UPROPERTY() float DashTimeScale = 0.12f;
	UPROPERTY() float GameOverTimeScale = 0.25f;
	UPROPERTY() float TimeScaleRate = 16.f;

	// The dash, the beat and the snip (real time)
	UPROPERTY() float DashSpeed = 6000.f;
	UPROPERTY() float BladePad = 20.f;
	UPROPERTY() float Beat = 0.3f;
	UPROPERTY() float Cooldown = 0.12f;
	UPROPERTY() int32 ScorePerSnip = 100;

	// The halves of a snipped plushie (toy: 70-160 px/s apart, 50 px/s along the cut, spin, gone after 6 s)
	UPROPERTY() float HalfSpeedMin = 250.f;
	UPROPERTY() float HalfSpeedMax = 560.f;
	UPROPERTY() float HalfSpeedAlong = 175.f;
	UPROPERTY() float HalfPop = 220.f;        // upward, so they tumble
	UPROPERTY() float HalfSpin = 200.f;       // degrees per second
	UPROPERTY() float HalfDrag = 2.8f;        // linear damping (toy: speed x0.06 per second)
	UPROPERTY() float HalfLife = 6.f;         // game seconds
	UPROPERTY() float HalfCapUpShare = 0.33f; // share of halves tipped to land cut side up; the rest settle at a natural tilt
	UPROPERTY() int32 MaxPieces = 60;

	// Juice
	UPROPERTY() float ShakeUnits = 3.5f;
	UPROPERTY() float BloodGravity = 1800.f;
	UPROPERTY() int32 MaxStains = 3000;      // small stains where drops land; the oldest are reused past this
	UPROPERTY() int32 MaxSplats = 4500;      // the splats under a snip (9 each): enough for a whole night

	// Pincushions (stationary hazards: cutting through one costs a heart)
	UPROPERTY() float PinRadius = 46.f;
	UPROPERTY() float PinLife = 12.f;        // game seconds
	UPROPERTY() float PinMinDistance = 280.f; // from her, when placed
	UPROPERTY() float PinEdge = 140.f;       // kept this far inside the mat's edge
	// T7b: the golden pincushion (a rare surprise): costs a heart like any, but scores big and counts in the combo
	UPROPERTY() float GoldenPinChance = 0.02f;
	// T7c: the blinking pincushion (purple): from night BlinkFromNight (Thursday = 4; Extended cut from round
	// BlinkFromRound) BlinkChance of pincushions blink: every BlinkEvery game seconds it vanishes and reappears
	// elsewhere; BlinkWarn seconds before, a faint ring marks the spot and pulses BlinkPulses times (a low tick each)
	UPROPERTY() float BlinkChance = 0.25f;
	UPROPERTY() int32 BlinkFromNight = 4;
	UPROPERTY() int32 BlinkFromRound = 20;
	UPROPERTY() float BlinkEvery = 4.f;
	UPROPERTY() float BlinkWarn = 1.f;
	UPROPERTY() int32 BlinkPulses = 2;
	UPROPERTY() float BlinkLight = 30.f;     // T7d: in the dark, the pulsing light on the spot (candela at each pulse)
	UPROPERTY() int32 GoldenPinScore = 3500;
	// T7d: variants: from night VariantFromNight (Extended cut: round VariantFromRound) VariantChance of spawns become one
	UPROPERTY() float VariantChance = 0.04f;
	UPROPERTY() int32 VariantFromNight = 4;
	UPROPERTY() int32 VariantFromRound = 30;
	// T7d: the impostor: from night ImpostorFromNight, ImpostorChance of the non-boss, non-dark waves get one (Extended
	// cut: from round ImpostorFromRound, ImpostorRoundChance); it sits ImpostorSit game seconds (wiggling), then walks
	// at ImpostorSpeed to the edge that was nearest; spawned at least ImpostorEdge from every edge; GoldImpostorChance gold
	UPROPERTY() float ImpostorChance = 0.4f;
	UPROPERTY() int32 ImpostorFromNight = 5;
	UPROPERTY() int32 ImpostorFromRound = 35;
	UPROPERTY() float ImpostorRoundChance = 0.3f;
	UPROPERTY() float ImpostorSit = 1.5f;
	UPROPERTY() float ImpostorSpeed = 60.f;
	UPROPERTY() float ImpostorEdge = 450.f;
	UPROPERTY() float GoldImpostorChance = 0.1f;

	// T7b juice: hitstop (a short freeze of the world at the snip, real seconds) and the snip's screen shake
	UPROPERTY() float HitStop = 0.035f;        // a snip of 1
	UPROPERTY() float HitStopPer = 0.012f;     // + this per plushie more
	UPROPERTY() float HitStopMax = 0.1f;
	UPROPERTY() float HitStopFinale = 0.2f;    // the boss's final split
	UPROPERTY() float HitStopScale = 0.02f;    // the world's time scale during it
	UPROPERTY() float SnipShake = 4.f;         // a snip of 1 (shake units, x shakeUnits uu)
	UPROPERTY() float SnipShakePer = 1.5f;     // + this per plushie more
	UPROPERTY() float BigSnipShake = 14.f;     // a snip of bigSnipCount or more
	UPROPERTY() int32 BigSnipCount = 4;
	UPROPERTY() float SnipShakeMax = 20.f;
	UPROPERTY() float FinaleShake = 30.f;      // the boss's final split
	UPROPERTY() float TrailFade = 0.6f;
	UPROPERTY() int32 CameraCutAt = 5;          // T7d: a line through this many cuts the camera to her (if Camera cuts is on)
	UPROPERTY() float PlushSelfLight = -1.f;   // T7b: plush fabric self-light (< 0 = M_KPPlush's own 0.12)
	UPROPERTY() float TheatricalStuffGlow = -1.f; // T7b: the Theatrical stuffing's glow (< 0 = M_KPCap's own 0.35)        // the dash trail: how much thinner its old end is than its new end (0-1)
	UPROPERTY() float HeroSelfLight = 0.22f;      // T7f: the heroine model's self-light (M_KPHeroine), so she reads at night
	UPROPERTY() float HeroHeight = 240.f;         // T7f: her height in uu (the model is scaled to it; the hitbox stays heroRadius)

	// Between waves
	UPROPERTY() float PickDelay = 0.7f;      // real seconds after a wave is cleared before the cards show
	UPROPERTY() int32 MaxHearts = 5;

	// Lights out (Thursday): the room lights drop to LightsOutDim and a warm lamp follows her
	UPROPERTY() float LightsOutDim = 0.06f;
	UPROPERTY() float LampHeight = 900.f;
	UPROPERTY() float LampRadius = 500.f;     // lit circle on the mat
	UPROPERTY() float LampIntensity = 1400.f; // candela
	UPROPERTY() float LampSilhouette = 0.f;   // plush self-light outside the circle (0: dark)
	UPROPERTY() float EyeGlint = 0.35f;        // brightness of the button eyes' shine in the dark (1 = white)
	UPROPERTY() float ChalkLightReach = 250.f;  // the chalk line lights the mat this far to each side (Lights out)
	UPROPERTY() float ChalkLightFade = 1.5f;    // real seconds it takes to fade after the cut
	UPROPERTY() float ChalkLightIntensity = 40.f; // candela per light along the line
	UPROPERTY() float ChalkLightSpacing = 220.f;  // one light per this much line
	UPROPERTY() int32 ChalkLights = 8;            // at most this many lights along a line

	// Pincushions placed in her way (nights with pinsInPath > 0): the first after this many game seconds of a wave, then one every pinPathEvery
	UPROPERTY() float PinPathFirst = 1.5f;
	UPROPERTY() float PinPathEvery = 2.5f;

	// Cards
	UPROPERTY() float ExtraInch = 120.f;      // (unused since T4c: Extra inch is a cone, below)
	UPROPERTY() TArray<float> ExtraInchAngles; // Extra inch: the cone ahead of the line's end, full angle in degrees per card level (I, II, III)
	UPROPERTY() TArray<float> ExtraInchLengths; // and its length per level
	UPROPERTY() float SeamReach = 150.f;       // Seam ripper: neighbours within this of a cut plushie's edge (per level)
	UPROPERTY() float SeamPinTime = 0.22f;    // Seam ripper: real seconds a pin flies to its target
	UPROPERTY() int32 SeamPinsPerTarget = 3;

	// The end of a night
	UPROPERTY() float UiClickDelay = 0.6f;    // real seconds before the results screen takes a click

	// Hardcore (T6, after the week is complete): hearts carry over between nights, losing them all goes back to Monday
	UPROPERTY() float HardcoreScore = 1.5f;   // (unused since T6c: modes have no multipliers)
	// T6c: darkness after Thursday: DarkWaves waves across the nights after the Lights out night, at most
	// DarkPerNight in one night, never a boss wave; rolled when a week starts and saved with it
	UPROPERTY() int32 DarkWaves = 3;
	UPROPERTY() int32 DarkPerNight = 2;
	UPROPERTY() int32 AliveCap = 45;          // T6c: at most this many plushies alive at once (spawns wait)
	UPROPERTY() float Countdown = 3.f;        // T6c: the 3-2-1 before every night, in real seconds

	// The title screen (T6): plushies wander in behind the menu and she cuts them by herself
	UPROPERTY() int32 TitlePlushies = 7;      // at most this many on the mat
	UPROPERTY() float TitleSpeed = 55.f;      // their speed (times the type's speedMul)
	UPROPERTY() float TitleSpawnEvery = 0.9f; // game seconds between them
	UPROPERTY() float TitleCutEvery = 2.4f;   // real seconds between her cuts
	UPROPERTY() TMap<FString, float> TitleMix; // type name -> weight (every type once, so their materials are ready before the first night)
};

/** One kind of plushie (Content/Data/KillingPart.json, "types"). Speeds multiply the wave's speed. */
USTRUCT()
struct FKPPlushieType
{
	GENERATED_BODY()

	UPROPERTY() FString Name = TEXT("teddy");
	UPROPERTY() float RadiusMin = 42.f;
	UPROPERTY() float RadiusMax = 56.f;
	UPROPERTY() float SpeedMul = 1.f;
	UPROPERTY() float ScoreMul = 1.f;        // its share of a snip's score (golden: 5)
	UPROPERTY() FString Ears = TEXT("round"); // round or long
	UPROPERTY() TArray<FString> Colors;      // "#RRGGBB"; empty = the six pastels
	UPROPERTY() float SelfLight = -1.f;      // plush self-light override (golden glows a bit); -1 = the material's

	// Hopping (bunnies): stand still for HopPause, then hop for HopTime, covering the same ground on average
	UPROPERTY() bool Hop = false;          // (no b prefix: the JSON key is "hop")
	UPROPERTY() float HopPause = 0.7f;
	UPROPERTY() float HopTime = 0.35f;
	UPROPERTY() float HopHeight = 45.f;

	// Splitting (big bears): after SplitDelay game seconds its two halves get up as two SplitInto plushies
	UPROPERTY() FString SplitInto;
	UPROPERTY() float SplitDelay = 0.45f;

	// Ignores slow motion (patchwork): walks on real time while she draws and during the beat
	UPROPERTY() bool IgnoreSlowMo = false;
	UPROPERTY() FString Look;                // "" (plain), "patchwork" (patches, an X-stitched eye, jitter), "hen"
	UPROPERTY() FString Intro;               // shown once, the first time one appears ("Patchwork: it doesn't slow down")

	// Laying (mother hen): every LayEvery game seconds she drops a Lays plushie behind her
	UPROPERTY() FString Lays;
	UPROPERTY() float LayEvery = 3.f;
	UPROPERTY() int32 ArrivesWith = 0;        // this many of Lays burst out as it spawns and rush straight at her
	// T7c: keeping her distance (mother hen): walks in until KeepAway uu from her, then circles at CircleMul x its
	// speed, backing away at BackAwayMul x its speed when she's nearer than KeepAway - KeepAwayBand; stays on the mat
	UPROPERTY() float KeepAway = 0.f;         // 0 = walks straight at her like the others
	UPROPERTY() float KeepAwayBand = 80.f;
	UPROPERTY() float CircleMul = 0.8f;
	UPROPERTY() float BackAwayMul = 2.5f;

	// The boss (T6, Sunday's last wave): every cut takes a chunk off; the last one splits it in half
	UPROPERTY() bool Boss = false;
	UPROPERTY() float BossSpeed = 40.f;       // uu/s (cards that speed plushies up apply too)
	UPROPERTY() int32 BossCuts = 6;           // the last of these splits it
	UPROPERTY() float ChunkOffset = 0.6f;     // the chunk's cut plane, in radii from the centre on the side the line passed
	UPROPERTY() int32 ChunkScore = 500;       // per chunk (it counts in the line's combo too)
	UPROPERTY() int32 SplitScore = 3000;      // the final split
	UPROPERTY() float Knockback = 300.f;      // how far it's pushed back when it touches her (costs a heart)
	UPROPERTY() float ShrinkPerCut = 0.06f;   // it gets this much smaller with every chunk
	// T6b: every chunk cut off gets up as a ChunkInto plushie after ChunkDelay game seconds and rushes at her at ChunkSpeed
	UPROPERTY() FString ChunkInto;
	UPROPERTY() float ChunkDelay = 0.5f;
	UPROPERTY() float ChunkSpeed = 200.f;
	// T6b: with EnrageAt cuts left it enrages: speed x EnrageSpeed, and it shakes with a glinting eye for EnrageTell real seconds
	UPROPERTY() int32 EnrageAt = 20;           // (T6c: hits left)
	// T6c: the real boss. BossHits lines to kill it (one line = one hit, HitScore each); a chunk comes off every
	// ChunkEvery hits; every hit brings HitSpawns plushies (HitPinShare of them pincushions) from the edges;
	// ChickCount chicks come in every ChickEvery game seconds while it lives
	UPROPERTY() int32 BossHits = 50;
	UPROPERTY() int32 ChunkEvery = 10;
	UPROPERTY() int32 HitScore = 100;
	UPROPERTY() int32 HitSpawns = 1;
	UPROPERTY() float HitPinShare = 0.2f;
	UPROPERTY() float ChickEvery = 3.f;
	UPROPERTY() int32 ChickCount = 7;
	UPROPERTY() float EnrageSpeed = 1.5f;
	UPROPERTY() float EnrageTell = 1.f;

	// T7d: a variant (two types' traits in one): rare, only once both Needs types have shown up this week
	// (Extended cut: are in the round's mix); Label is its name in the "New: ..." toast
	UPROPERTY() bool Variant = false;
	UPROPERTY() TArray<FString> Needs;
	UPROPERTY() FString Label;
	// T7d: the impostor (a plushie looking like a pincushion, with felt feet): see AKPDirector's impostor code
	UPROPERTY() bool Impostor = false;
	UPROPERTY() bool GoldLook = false;        // gold fabric and pins, the sparkle (the golden impostor)
};

/** One wave of a night. Plushie speed = Speed + 0..plushieSpeedRandom, times the type's speedMul. */
USTRUCT()
struct FKPWave
{
	GENERATED_BODY()

	UPROPERTY() int32 Count = 5;
	UPROPERTY() float Speed = 74.f;
	UPROPERTY() float Interval = 0.45f;      // game seconds between spawns
	UPROPERTY() int32 Pins = 0;              // pincushions placed when it starts
	UPROPERTY() TMap<FString, float> Mix;    // type name -> weight; empty = the night's mix
	UPROPERTY() TMap<FString, int32> Extra;  // type name -> how many more of exactly that type (a teaser), mixed into the second half of the wave
	UPROPERTY() FString Boss;                // T6: a boss type that walks in when the wave starts
	UPROPERTY() float EscortEvery = 2.8f;    // while the boss lives: one plushie from the night's mix every this many game seconds
	UPROPERTY() int32 BossCount = 1;         // T6c: this many bosses at once (Extended cut)
	UPROPERTY() int32 BossHits = 0;          // T6c: hits per boss (0 = the type's bossHits)
};

/** One night (Monday to Sunday): a rule and a list of waves */
USTRUCT()
struct FKPNight
{
	GENERATED_BODY()

	UPROPERTY() FString Name = TEXT("Monday");
	UPROPERTY() FString Rule;                // what the code switches on: lightsout (the others are data only)
	UPROPERTY() FString RuleName;            // shown big at the start, e.g. "Pins"
	UPROPERTY() FString RuleText;            // one line under it
	UPROPERTY() float GoldenChance = 0.f;
	UPROPERTY() float ChalkMul = 1.f;
	UPROPERTY() float PinsInPath = 0.f;      // share of each wave's pincushions placed between her and the nearest plushies
	UPROPERTY() TMap<FString, float> Mix;
	UPROPERTY() TArray<FKPWave> Waves;
};

/** An upgrade card: what it does is in code (AKPDirector::ApplyCards, by Id); the words are here */
USTRUCT()
struct FKPCard
{
	GENERATED_BODY()

	UPROPERTY() FString Id;
	UPROPERTY() FString Name;
	UPROPERTY() FString Boon;
	UPROPERTY() FString Thread;              // the loose thread: the catch
	UPROPERTY() bool Pool = true;            // false: parked, never offered (kp.Card still takes it)
	UPROPERTY() TMap<FString, float> Values; // the card's numbers (per level); "{key}" in Boon/Thread shows them
	// T7c: a curse (Impossible, Godlike): a loose thread on its own, no boon, never in the card pool. Its values
	// speed (plushies +%), chalk (-%), plushies (+% per wave) and pincushions (+n per wave) apply for the night
	UPROPERTY() bool Curse = false;
};

namespace KPCardText
{
	/** "Extra inch", "Extra inch II", "Extra inch III" */
	inline FString Title(const FString& Name, int32 Level)
	{
		return Level >= 3 ? Name + TEXT(" III") : Level == 2 ? Name + TEXT(" II") : Name;
	}
}

USTRUCT()
struct FKPModeInfo
{
	GENERATED_BODY()

	// T6b: a game mode (KillingPart.json "modes"). Normal is the week as it is; the others are runs from Monday
	// with their own numbers and their own week best
	UPROPERTY() FString Id = TEXT("normal");
	UPROPERTY() FString Name = TEXT("Normal");
	UPROPERTY() FString Rule;                // (before T6d: one line on its card; no longer shown)
	UPROPERTY() TArray<FString> Rules;       // T6d: its complete rules as short bullets, shown on its card
	UPROPERTY() bool CarryHearts = false;    // hearts carry over between nights; losing them all goes back to Monday
	UPROPERTY() int32 CardsOffered = 3;      // per card pick: 3, 1 (given: click it to take it) or 0 (no picks)
	UPROPERTY() bool NightCard = false;      // T6c: one random card at the start of each night (Extreme)
	UPROPERTY() int32 Hearts = 0;            // T6c: hearts at the start of a week (0 = tuning's hearts; Final cut: 1)
	UPROPERTY() FString Unlock;              // "" (from the start) or the mode id whose week must be finished
	UPROPERTY() FString Locked;              // why it's locked ("Finish Hardcore to unlock")
	UPROPERTY() bool Endless = false;        // T6c: Extended cut: endless rounds from "endless" instead of the week
	UPROPERTY() bool Dark = false;           // T6c: every round is dark (Extended nightmare)
	UPROPERTY() int32 UnlockRound = 0;       // T6c: unlocked by finishing this round in the Unlock mode instead of finishing it (T6d: finishing, not reaching)
	UPROPERTY() int32 Curses = 0;            // T7c: at each night's start, pick 1 of this many curses (1 = one random curse, no choice; 0 = none)
};

/** T6c: Extended cut's endless rounds, all from formulas (KillingPart.json "endless") */
USTRUCT()
struct FKPEndless
{
	GENERATED_BODY()

	UPROPERTY() int32 PlushiesBase = 3;        // plushies(r) = base + perRound x (r - 1)
	UPROPERTY() int32 PlushiesPerRound = 1;
	UPROPERTY() float SpeedBase = 74.f;        // speed(r) = base + step x r, at most max
	UPROPERTY() float SpeedStep = 4.f;
	UPROPERTY() float SpeedMax = 230.f;
	UPROPERTY() float IntervalStart = 0.45f;   // spawn interval(r) = start - step x r, at least min
	UPROPERTY() float IntervalStep = 0.005f;
	UPROPERTY() float IntervalMin = 0.22f;
	UPROPERTY() int32 NewTypeEvery = 5;        // a new type joins every this many rounds, in TypeOrder ("pincushion" = pincushions)
	UPROPERTY() TArray<FString> TypeOrder;
	UPROPERTY() float TypeWeight = 0.5f;       // a joined type's share in the mix (the teddy's is 1)
	UPROPERTY() float HarderStep = 0.15f;      // after the last joins, every NewTypeEvery rounds the joined types' shares grow by this
	UPROPERTY() int32 PinsEvery = 5;           // pincushions(r) = 1 + (r - joined) / every, at most max
	UPROPERTY() int32 PinsMax = 4;
	UPROPERTY() float PinsInPath = 0.34f;
	UPROPERTY() int32 BossFrom = 30;           // from this round every BossEvery-th round is a boss round
	UPROPERTY() int32 BossEvery = 10;
	UPROPERTY() int32 BossCountEvery = 20;     // bosses(r) = 1 + (r - from) / this
	UPROPERTY() int32 BossHitsBase = 15;       // hits per boss = base + step x (boss rounds so far)
	UPROPERTY() int32 BossHitsStep = 5;
	UPROPERTY() FString BossType = TEXT("first");
	UPROPERTY() int32 CardEvery = 10;          // a card pick after every this many rounds
	UPROPERTY() float GoldenChance = 0.025f;
};

/** T7: one sound role (the snip, a squish, the night music...): its options and how it plays */
USTRUCT()
struct FKPSoundRole
{
	GENERATED_BODY()

	UPROPERTY() FString Label;               // its name in the sound test
	UPROPERTY() TArray<FString> Options;     // asset names in /Game/KillingPart/Audio; option a, b, c
	UPROPERTY() FString Pick = TEXT("a");    // the option used (the sound test's local pick wins over it)
	UPROPERTY() float Volume = 1.f;
	UPROPERTY() float Pitch = 1.f;
	UPROPERTY() float PitchRandom = 0.06f;   // +- this share, per play
	UPROPERTY() float MinGap = 0.f;          // real seconds: a play this soon after the last one is skipped
	UPROPERTY() bool Ui = false;             // plays while paused and on the title (others are scaled by titleSfx there)
};

/** T7: the mix (KillingPart.json "audio") */
USTRUCT()
struct FKPAudioData
{
	GENERATED_BODY()

	UPROPERTY() TMap<FString, FKPSoundRole> Sounds;
	UPROPERTY() float MusicVolume = 0.5f;      // the music under everything (times the player's Music setting)
	UPROPERTY() float SfxVolume = 1.f;         // every sound effect (times the player's Sounds setting)
	UPROPERTY() float Crossfade = 1.5f;        // seconds between the menu music and the night music
	UPROPERTY() float DrawLowPassHz = 700.f;   // while drawing (slow motion) the music is muffled...
	UPROPERTY() float DrawMusicVolume = 0.5f;  // ...and drops to this share
	UPROPERTY() float LowPassFade = 0.2f;      // seconds into and out of it
	UPROPERTY() float DarkLowPassHz = 1200.f;  // a dark wave: muffled the whole wave
	UPROPERTY() float DarkMusicVolume = 0.8f;
	UPROPERTY() FString DarkMusic = TEXT("night"); // "menu": dark waves play the menu music instead
	UPROPERTY() int32 SquishesPerSnip = 3;     // a big cut: one snip and at most this many squishes...
	UPROPERTY() float SquishSpread = 0.045f;   // ...this far apart (real seconds)...
	UPROPERTY() float SquishFalloff = 0.7f;    // ...each one this share of the one before
	UPROPERTY() int32 SqueaksPerSnip = 2;
	UPROPERTY() float SqueakChance = 0.45f;    // per plushie cut, until squeaksPerSnip
	UPROPERTY() float SqueakDelayMin = 0.06f;
	UPROPERTY() float SqueakDelayMax = 0.28f;
	UPROPERTY() bool ChalkLoop = true;         // the chalk loop while drawing
	UPROPERTY() float ChalkFade = 0.08f;
	UPROPERTY() float TitleSfx = 0.f;          // game sounds on the title (the mat playing itself)
	UPROPERTY() int32 KillingPartCombo = 5;    // a combo this big plays killingpart
	UPROPERTY() TArray<float> ResultsBeats;    // sfx_results' hits (seconds); the stat lines appear on them
	UPROPERTY() float EndingResultsAt = 1.6f;  // the ending: sfx_results starts this long after the screen opens
	UPROPERTY() float RunOverResultsAt = 0.4f; // the Extended cut's run-over screen
	// T7b: night music: musicNight's options play one after another (a, then b, then a...) instead of looping;
	// a new wave while a later one plays crossfades back to the first over waveCrossfade seconds
	UPROPERTY() float WaveCrossfade = 1.5f;
	// T7b: losing (sfx_gameover): the night music fades out over gameOverFade s; the menu music comes in after gameOverQuiet s
	UPROPERTY() float GameOverFade = 0.8f;
	UPROPERTY() float GameOverQuiet = 3.0f;
};

/** T7d: the wardrobe's swatches per row ("#RRGGBB"; the first is the default). Hair and Bow go in pairs (one row). */
USTRUCT()
struct FKPWardrobe
{
	GENERATED_BODY()

	UPROPERTY() TArray<FString> Dress;
	UPROPERTY() TArray<FString> Apron;
	UPROPERTY() TArray<FString> Hair;
	UPROPERTY() TArray<FString> Bow;
	UPROPERTY() TArray<FString> Shears;
	UPROPERTY() TArray<FString> Trail;
};

USTRUCT()
struct FKPDataFile
{
	GENERATED_BODY()

	UPROPERTY() FKPWardrobe Wardrobe;         // T7d

	UPROPERTY() FKPAudioData Audio;           // T7

	UPROPERTY() FKPTuning Tuning;
	UPROPERTY() TArray<FKPPlushieType> Types;
	UPROPERTY() TArray<FKPNight> Nights;
	UPROPERTY() TArray<FKPCard> Cards;
	UPROPERTY() TArray<FKPModeInfo> Modes;    // T6b: game modes
	UPROPERTY() FKPEndless Endless;           // T6c: Extended cut's rounds
	UPROPERTY() TMap<FString, FString> Texts; // the results screen's words (KPDirector's Text(key, fallback))
};

namespace KPData
{
	/** Reads Content/Data/KillingPart.json. Keeps the defaults (and logs why) if the file is missing or broken. */
	FKPDataFile Load();
}

namespace KPSettings
{
	/**
	 * Which version is on screen. The Director's cut (the default) is the gory one. The Theatrical cut
	 * has no blood at all: white stuffing inside the plushies, white fluff bursts, no splats.
	 * Global for now (console: kp.Cut theatrical|director); T5 saves it and T6 adds the menu toggle.
	 */
	bool IsTheatrical();
	void SetTheatrical(bool bOn);
}

namespace KPVisuals
{
	UStaticMesh* SphereMesh();
	UStaticMesh* CubeMesh();
	UStaticMesh* CylinderMesh();

	/** A lit colour (BasicShapeMaterial, which has a Color parameter) */
	UMaterialInstanceDynamic* Lit(UObject* Outer, const FLinearColor& Color);

	/** An unlit colour (/Game/KillingPart/Materials/M_KPUnlit; falls back to lit if that asset is missing) */
	UMaterialInstanceDynamic* Unlit(UObject* Outer, const FLinearColor& Color);

	/**
	 * Plush fabric tinted by Color (/Game/KillingPart/Materials/M_KPPlush: the PBRgen minky maps, Cloth
	 * shading with a lighter fuzz rim; falls back to Lit if that asset is missing). For the plushie bodies
	 * and ears; the halves of a cut body keep it on their outside.
	 */
	UMaterialInstanceDynamic* Plush(UObject* Outer, const FLinearColor& Color);

	/** Sets the colour of a material made by Lit, Unlit or Plush */
	void SetColor(UMaterialInstanceDynamic* Material, const FLinearColor& Color);

	/** "#RRGGBB" in sRGB to linear */
	FLinearColor Hex(const TCHAR* Text);

	/** T7b: the plush fabric's self-light (pastels under the night light; < 0 = M_KPPlush's own) and the Theatrical
	 * stuffing's glow (< 0 = M_KPCap's own), set from tuning plushSelfLight / theatricalStuffGlow */
	extern float PlushSelfLight;
	extern float TheatricalStuffGlow;
	/** T7f: the heroine's self-light and height (tuning heroSelfLight / heroHeight) */
	extern float HeroSelfLight;
	extern float HeroHeight;
}

namespace KPMath
{
	/** Distance in XY from P to the segment AB */
	float SegmentDistance2D(const FVector& P, const FVector& A, const FVector& B);

	/**
	 * The normal of a vertical cut along YawDegrees: cross(cut direction, up) = (sin, -cos, 0).
	 * The one definition for the hit test, the cut line and the slice (two different ones put thin
	 * slices on the wrong side).
	 */
	FVector CutNormal(float YawDegrees);

	/** The point on the segment AB closest to P, in XY (Z of the result is A's) */
	FVector ClosestPointOnSegment2D(const FVector& P, const FVector& A, const FVector& B);

	/** 0 to 1 with a small overshoot past 1 before settling: a "pop" */
	inline float EaseOutBack(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f) - 1.f;
		return 1.f + 2.70158f * T * T * T + 1.70158f * T * T;
	}
}

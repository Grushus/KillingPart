// Killing Part: a pincushion, the stationary hazard (DESIGN.md). A round red cushion with pins sticking
// out of it. Cutting through one costs a heart (the director checks that in its dash sweep). It pops in
// when placed and stays until the wave ends (FadeOut). No tick: the director drives it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KPPincushion.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class AKPPincushion : public AActor
{
	GENERATED_BODY()

public:
	AKPPincushion();

	/** Builds the look. Call once, right after spawning. bInGolden (T7b): gold fabric, gold pin heads and a sparkle. */
	void Init(float InRadius, float InLife, bool bInGolden = false, bool bInBlink = false);

	/** T7b: a golden pincushion (costs a heart like any, scores big) */
	bool bGolden = false;

	/** T7c: the blinking pincushion (purple): the director moves it every few seconds (AKPDirector::UpdatePins) */
	bool bBlink = false;
	float BlinkTimer = 0.f;         // game seconds to the next blink
	bool bBlinkAiming = false;      // its next spot is chosen and shown
	FVector BlinkTo = FVector::ZeroVector;
	int32 BlinkPulsesDone = 0;
	/** Vanishes and pops in at At */
	void Reappear(const FVector& At);
	bool IsFading() const { return Life < 1.f; }

	/** Counts its life down (game seconds) and animates it. Returns false when it is gone. */
	bool Advance(float GameDt);

	/** A shake when her blade goes through it */
	void Jolt() { JoltTime = 0.35f; }

	/** Starts shrinking away now (the wave is over) */
	void FadeOut();

	/** Lights out: 1 = lit as normal, 0 = in the dark (hidden completely) */
	void SetLit(float Amount);

	float Radius = 46.f;
	float Life = 12.f;
	bool bHit = false;   // already cost a heart (once per pincushion, like the toy)

private:
	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<USceneComponent> Visual;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Cushion;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Band;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Pins;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CushionMaterial;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Sparkles;   // T7b: the golden one's twinkles (unlit, so they glint in the dark too)
	TArray<FVector> SparkleSpots;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> BlinkHeadMaterials;   // T7d: the blinking one's pin heads (unlit: they glint in the dark)
	TArray<FLinearColor> BlinkHeadBase;
	TArray<float> SparklePhase;
	float Age = 0.f;
	float JoltTime = 0.f;
	float BaseSelfLight = 0.12f;
	float CurrentLit = 1.f;
};

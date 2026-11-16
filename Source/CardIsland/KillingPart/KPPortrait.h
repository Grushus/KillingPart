// Killing Part: the portrait for "meet a new plushie" (T6). A small stage far above the table: one
// plushie (or a pincushion, or a hen with a chick) turning slowly under a warm key light and a cool rim
// light, filmed by its own scene capture into a render target that the HUD draws over the blurred mat.
// Only the subject is rendered (show-only list), so the table and the mat never get in the way.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KPPortrait.generated.h"

class AKPPlushie;
class AKPPincushion;
class USceneCaptureComponent2D;
class USpotLightComponent;
class UPointLightComponent;
class UTextureRenderTarget2D;
class UStaticMeshComponent;
struct FKPPlushieType;

UCLASS()
class AKPPortrait : public AActor
{
	GENERATED_BODY()

public:
	AKPPortrait();

	virtual void Tick(float DeltaSeconds) override;

	/** Puts one plushie on the stage (and a companion beside it, e.g. a chick for the hen) */
	void ShowPlushie(const FKPPlushieType& Type, int32 TypeIndex, float Radius, const FLinearColor& Color,
		const FKPPlushieType* Companion, int32 CompanionIndex, float CompanionRadius, const FLinearColor& CompanionColor);

	/** Puts a pincushion on the stage */
	void ShowPincushion(float Radius, bool bBlink = false, bool bGolden = false);

	/** Empties the stage and stops capturing */
	void Clear();

	UTextureRenderTarget2D* GetTexture() const { return Target; }

private:
	void Frame(float SubjectRadius);
	FVector StageCenter() const { return GetActorLocation(); }

	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY() TObjectPtr<USpotLightComponent> KeyLight;
	UPROPERTY() TObjectPtr<UPointLightComponent> RimLight;
	UPROPERTY() TObjectPtr<UPointLightComponent> FillLight;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Floor;   // a dark felt disc: the key light's pool on it is the spotlight
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Target;
	UPROPERTY() TObjectPtr<AKPPlushie> Subject;
	UPROPERTY() TObjectPtr<AKPPlushie> Companion;
	UPROPERTY() TObjectPtr<AKPPincushion> Pin;
	float Clock = 0.f;
	float SubjectSize = 50.f;
	bool bHopper = false;
};

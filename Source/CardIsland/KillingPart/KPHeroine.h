// Killing Part: the seamstress. A pawn with no movement of her own: AKPDirector places her.
// T7f: she is Simon's Tripo model (SM_Heroine, one static mesh with a baked texture, material M_KPHeroine) with
// procedural life: idle bob and breathing, a lean and stretch into the dash, a squash when it ends, a hurt shake
// and flash; she turns smoothly to her facing. Giant shears made of basic shapes sit at her right hand.
// The T7d/T7e mannequin look (Quinn, procedural dress, outfit parts) is kept behind KPHeroineLook::bMannequin
// (false): not loaded, not shown.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "KPHeroine.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UAnimSequence;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;

UCLASS()
class AKPHeroine : public APawn
{
	GENERATED_BODY()

public:
	AKPHeroine();

	virtual void BeginPlay() override;

	/** Places her on the mat facing Yaw (degrees) */
	void Place(const FVector& Location, float Yaw);

	/** Opening angle of the shears in radians (each blade turns by this much) */
	void SetShearsOpen(float Angle);

	/** The shears only exist during the dash and the beat */
	void SetShearsVisible(bool bShow);

	void PlayIdle();
	void PlayDash();

	/** Goes back to idle once a one-shot animation has finished (the mannequin only) */
	void UpdateAnimation();

	/** T7f: her procedural life, every frame in real time: turning, bob, breathing, dash lean/stretch, squash, hurt */
	void Animate(float RealDt);

	/** T7f: she's been hit: a quick shake and a red flash */
	void Hurt();

	/** Hides her for the blink while she's invulnerable */
	void SetBlinkHidden(bool bHide);

	/** T7d: her colours (the wardrobe): dress, apron, hair, bow and the shears' handles */
	void SetLook(const FLinearColor& Dress, const FLinearColor& Apron, const FLinearColor& Hair, const FLinearColor& Bow, const FLinearColor& Handles);

private:
	void PlayOnce(UAnimSequence* Sequence, bool bLoop);
	void BuildShears();
	void BuildOutfit();
	void ApplyLook();
	/** Puts Part on bone BoneName so that it sits at Location / Rotation / Scale in the mesh's own (reference pose) space */
	void PlaceOnBone(USceneComponent* Part, const FName& BoneName, const FVector& Location, const FRotator& Rotation, const FVector& Scale);
	FVector BoneRef(const FName& BoneName) const;
	/** T7e: the dress (bell skirt, apron, waistband) and the bodice, procedural, in the mesh's own space */
	void BuildDress();

	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<USceneComponent> Lean;          // T7f: her smoothed facing, lean, bob and shake (the model and the shears hang on it)
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Model;    // T7f: SM_Heroine (squash and stretch go on its scale)
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ModelMaterial;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY() TObjectPtr<USceneComponent> Shears;
	UPROPERTY() TObjectPtr<USceneComponent> BladePivotA;
	UPROPERTY() TObjectPtr<USceneComponent> BladePivotB;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ShearParts;     // per side: blade, shank, ring segments
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ShearScrew;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> OutfitParts;    // see BuildOutfit for the order
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Skirt;   // T7e: sections 0 skirt, 1 apron, 2 waistband (on the pelvis)
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Bodice;  // T7e: the top of the dress (on spine_03)

	UPROPERTY() TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> DashAnim;

	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> DressMaterial;   // T7e: the skirt (fabric)
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BodiceMaterial;  // T7e: a lighter shade of the dress
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ShoeMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> EyeMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SkinMaterialA;   // T7e: the mannequin's two materials, both skin
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SkinMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ApronMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> HairMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BowMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> HandleMaterial;

	FLinearColor DressColor = FLinearColor::Black;
	FLinearColor ApronColor = FLinearColor::White;
	FLinearColor HairColor = FLinearColor::White;
	FLinearColor BowColor = FLinearColor::White;
	FLinearColor HandleColor = FLinearColor::White;
	bool bLookSet = false;

	bool bPlayingIdle = false;
	bool bBlinkHidden = false;

	// T7f: procedural life
	bool bDashing = false;
	bool bYawSet = false;
	float VisualYaw = 0.f;
	float LifeClock = 0.f;
	float DashBlend = 0.f;      // 0 idle .. 1 dashing
	float SquashTime = -1.f;    // seconds since the dash ended (< 0: no squash)
	float HurtTime = -1.f;      // seconds since she was hit (< 0: none)
	float SelfLightSet = -1.f;
	bool bShearsShown = true; // the constructor hides them; BeginPlay syncs this
};

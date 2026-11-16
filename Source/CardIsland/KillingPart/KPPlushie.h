// Killing Part: one plushie. The body is a procedural mesh (so it can be sliced at the snip); ears,
// eyes and the seam are basic shapes attached to it. It has no brain: AKPDirector moves it, marks it
// and pops it. Local +X is its face.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KPPlushie.generated.h"

class UStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
struct FKPTuning;
struct FKPPlushieType;

UCLASS()
class AKPPlushie : public AActor
{
	GENERATED_BODY()

public:
	AKPPlushie();

	/** Builds the look for one type (see FKPPlushieType). Call once, right after spawning. */
	void Init(const FKPPlushieType& Type, int32 InTypeIndex, float InRadius, const FLinearColor& InColor, float InSpeed);

	/**
	 * Faces the plushie and applies its waddle (Phase in radians; bWaddle false keeps it upright, for a
	 * hopper), lifted by Lift (a hop) and scaled by Scale (popping in).
	 */
	void UpdatePose(const FVector& FaceDirection, float Phase, bool bWaddle, float Lift, float Scale);

	/** The plush self-light, for the Lights out night (outside her lamp they go dark) */
	void SetSelfLight(float Value);
	float GetBaseSelfLight() const { return BaseSelfLight; }

	/** The shine on its button eyes: 1 is white (normal), lower is a faint glint (in the dark) */
	void SetEyeGlint(float Brightness);

	/** Added to the pose's position each UpdatePose (the patchwork's jitter) */
	FVector PoseJitter = FVector::ZeroVector;

	/** Seam ripper: a pin stays stuck in it (and in its half after the cut). Tip and direction in world space. */
	void AddStuckPin(const FVector& Tip, const FVector& Direction, float Length, const FLinearColor& HeadColor, float ShaftScale = 0.09f, float HeadScale = 0.34f, bool bLitHead = false);

	// T7d: the impostor (looks like a pincushion, with felt feet): the director walks it to ImpostorExit
	bool bImpostor = false;
	float ImpostorAge = 0.f;
	FVector ImpostorExit = FVector::ZeroVector;

	// A chick's scatter hop away from the hen: from ScatterFrom to ScatterTo while ScatterT runs 0 to 1
	FVector ScatterFrom = FVector::ZeroVector;
	FVector ScatterTo = FVector::ZeroVector;
	float ScatterT = -1.f;

	/** Where the two halves of a cut body are now (world), for a big bear getting up as two */
	bool GetHalfLocations(FVector& OutA, FVector& OutB) const;

	/**
	 * Freezes it with a white cut line across it, along InCutYaw (degrees, world), at InOffset from its
	 * centre along the cut normal (cross(cut direction, up)): where her path passed closest. Called again
	 * during the same dash when a later stretch of the path passes closer.
	 */
	void SetMarked(float InCutYaw, float InOffset);

	/** Distance in XY from her path to the centre when it was marked (to keep the closest pass) */
	float CutDistance = 0.f;

	/** Centre of the body in world space */
	FVector GetBodyCenter() const;

	/**
	 * The snip: cuts the body in two along the marked cut (a vertical plane through its centre, normal =
	 * cross(cut direction, up)) and sends the halves flying as physics bodies. The cap is a dynamic
	 * instance of CapBase (M_KPCap: skin, stuffing and insides in bands from the rim) set up for this
	 * plushie's size and colour. Ears, eyes and stitches go with the half they sit on.
	 * Returns false if the slice didn't work.
	 */
	bool Slice(UMaterialInterface* CapBase, const FKPTuning& Tuning);

	/** Switches the insides of the halves between the Director's cut (blood) and the Theatrical cut (stuffing) */
	void SetCapTheatrical(bool bTheatrical);

	/** Shrinks the halves away at the end of their life: 1 is full size, 0 is gone. Stops their physics. */
	void SetPiecesScale(float Alpha);

	// The boss (T6): every cut takes a chunk off instead of cutting it in two
	/**
	 * Slices a chunk off the side Outward (horizontal, world) points to: the plane sits OffsetFraction of the
	 * radius out from the centre, or nearer if that side was already cut away. The chunk (with any ear, eye
	 * or stitch on it) flies off as a physics piece with a cap; the rest stays on. Returns where the chunk is.
	 */
	bool ChopChunk(const FVector& Outward, float OffsetFraction, UMaterialInterface* CapBase, const FKPTuning& Tuning, FVector& OutChunkAt);
	/** Back to walking after a chopping cut: no cut line, not marked */
	void Unmark();
	/** The boss shrinks a little with every chunk (1 = its full size) */
	/** T6b: the boss's chunk Index gets up: it's hidden (physics off) and OutAt is where it lay. False if it's gone. */
	bool RaiseChunk(int32 Index, FVector& OutAt);
	int32 GetNumChunks() const { return Chunks.Num(); }
	void SetSizeScale(float InScale);
	float GetSizeScale() const { return SizeScale; }

	bool bBoss = false;
	// T6c: a boss's own fight (several can be on the mat in Extended cut)
	int32 BossHitsLeft = 0;
	int32 BossHitsTaken = 0;
	bool bBossEnraged = false;
	float BossTell = 0.f;
	FVector KnockDir = FVector::ZeroVector;  // the boss is knocked back when it touches her
	float KnockLeft = 0.f;

	// State the director reads and writes
	int32 TypeIndex = 0;
	float HopClock = 0.f;     // game seconds into its pause-hop cycle
	float PopIn = 0.f;        // game seconds left of popping in (a split bear's cubs); frozen and harmless meanwhile
	float SplitTimer = -1.f;  // >= 0: a cut big bear whose halves get up when it runs out
	float LayTimer = -1.f;    // >= 0: a hen, game seconds until she lays
	float CircleDir = 1.f;    // T7c: which way a hen keeping her distance circles (+1 / -1; flips at the mat's edge)
	bool bPatchwork = false;
	float Radius = 50.f;
	float Speed = 60.f;
	float Wobble = 0.f;
	float CutYaw = 0.f;
	float CutOffset = 0.f;    // the slice plane's distance from the centre along the cut normal (clamped)
	bool bMarked = false;
	bool bDead = false;
	bool bCut = false;        // sliced: the actor now only carries its two halves
	float PieceLife = 0.f;    // game seconds until the halves are removed
	FLinearColor Color = FLinearColor::White;

private:
	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<USceneComponent> Visual;
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> BodyMesh;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> EarLeft;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> EarRight;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> EyeLeft;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> EyeRight;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ShineLeft;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ShineRight;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Stitches;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CutLine;
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> OtherHalf;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CapMaterial;   // shared by both halves
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FabricMaterial;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> PatchMaterials;   // patchwork: one per patch
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ShineMaterial;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CrossA;   // patchwork: the X-stitched eye
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CrossB;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Beak;     // hen
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Comb;     // hen
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> StuckPins;   // Seam ripper pins (shaft, head, shaft, head, ...)
	UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Chunks;  // the boss: pieces cut off it, lying on the mat
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Feet;        // T7d: the impostor's felt feet
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Sparkles;    // T7d: the golden impostor's twinkles
	TArray<FVector> FootBase;
	float SizeScale = 1.f;
	float MeshRadius = 50.f;   // the body mesh's radius (Radius shrinks with SizeScale; the mesh doesn't)
	void EnsureCapMaterial(UMaterialInterface* CapBase);
	float BaseSelfLight = 0.12f;
	float CurrentSelfLight = -1.f;
	float CurrentGlint = 1.f;
	bool bPiecesFrozen = false;

	float BodyHeight = 50.f;
};

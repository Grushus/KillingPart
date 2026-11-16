// Killing Part: procedural meshes. The plushie body is generated here (not copied from the engine's
// sphere, which can't be read in a packaged build) so SliceProceduralMesh can cut it at the snip.

#pragma once

#include "CoreMinimal.h"

class UProceduralMeshComponent;

namespace KPMeshGen
{
	constexpr int32 BodySegments = 32;   // around
	constexpr int32 BodyRings = 16;      // top to bottom

	/**
	 * Fills section 0 of Mesh with a UV sphere centred on the component origin, squashed to RadiusZ.
	 * Local +X is the face.
	 * UVs: u runs once around (0 and 1 meet in a seam at the back, 0.5 is the face, increasing to the
	 * viewer's right), v runs from 0 at the top to 1 at the bottom. The surface is about twice as long
	 * around as it is top to bottom, so a square fabric texture wants a tiling of about (4, 2).
	 * Normals are the true ellipsoid normals; tangents point along +u.
	 */
	void BuildBody(UProceduralMeshComponent* Mesh, float RadiusXY, float RadiusZ);

	/**
	 * The same body cut into patches (the patchwork plushie): PatchesU around by PatchesV top to bottom,
	 * every other row shifted by half a patch like brickwork. Each patch is its own mesh section (0 to
	 * NumPatches - 1), so each can carry its own fabric colour. Returns the number of sections made.
	 */
	int32 BuildPatchBody(UProceduralMeshComponent* Mesh, float RadiusXY, float RadiusZ, int32 PatchesU, int32 PatchesV);

	/**
	 * Points for the simple convex collision of one piece of a cut body: the part on the positive side
	 * of the plane dot(p, LocalNormal) = Offset. LocalNormal is the plane normal in the body's space and
	 * must be horizontal (the cut is always vertical); |Offset| must be less than RadiusXY.
	 * At most about 70 points, so cooking it is cheap.
	 */
	void BuildPieceHull(float RadiusXY, float RadiusZ, const FVector& LocalNormal, float Offset, TArray<FVector>& OutPoints);
}

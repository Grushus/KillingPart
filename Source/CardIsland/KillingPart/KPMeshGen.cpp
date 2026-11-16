// Killing Part: procedural meshes.

#include "KillingPart/KPMeshGen.h"
#include "ProceduralMeshComponent.h"

namespace KPMeshGenPrivate
{
	/** Adds a triangle wound so that it faces away from the origin (the engine's front face has (B-A)x(C-A) pointing inward) */
	void AddOutwardTriangle(TArray<int32>& Triangles, const TArray<FVector>& Vertices, int32 A, int32 B, int32 C)
	{
		const FVector Cross = FVector::CrossProduct(Vertices[B] - Vertices[A], Vertices[C] - Vertices[A]);
		const FVector Centroid = Vertices[A] + Vertices[B] + Vertices[C];
		if (FVector::DotProduct(Cross, Centroid) > 0.0)
		{
			Swap(B, C);
		}
		Triangles.Add(A);
		Triangles.Add(B);
		Triangles.Add(C);
	}
}

void KPMeshGen::BuildBody(UProceduralMeshComponent* Mesh, float RadiusXY, float RadiusZ)
{
	if (!Mesh)
	{
		return;
	}

	const int32 Segments = BodySegments;
	const int32 Rings = BodyRings;
	const int32 NumVertices = 2 * Segments + (Rings - 1) * (Segments + 1);

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FColor> Colors;
	TArray<int32> Triangles;
	Vertices.Reserve(NumVertices);
	Normals.Reserve(NumVertices);
	UVs.Reserve(NumVertices);
	Tangents.Reserve(NumVertices);
	Triangles.Reserve(Segments * (Rings - 1) * 6);

	auto AddVertex = [&](float U, float V)
	{
		// u = 0 is the back (-X); it increases to the viewer's right, so the azimuth runs backwards
		const float Azimuth = UE_PI - UE_TWO_PI * U;
		const float Polar = UE_PI * V;
		const float SinPolar = FMath::Sin(Polar);
		const FVector Unit(SinPolar * FMath::Cos(Azimuth), SinPolar * FMath::Sin(Azimuth), FMath::Cos(Polar));
		Vertices.Add(FVector(Unit.X * RadiusXY, Unit.Y * RadiusXY, Unit.Z * RadiusZ));
		Normals.Add(FVector(Unit.X / RadiusXY, Unit.Y / RadiusXY, Unit.Z / RadiusZ).GetSafeNormal());
		UVs.Add(FVector2D(U, V));
		Tangents.Add(FProcMeshTangent(FVector(FMath::Sin(Azimuth), -FMath::Cos(Azimuth), 0.f), false));
		return Vertices.Num() - 1;
	};

	// The poles get one vertex per segment (u at the middle of the segment) so the texture doesn't pinch
	const int32 TopBase = Vertices.Num();
	for (int32 Segment = 0; Segment < Segments; ++Segment)
	{
		AddVertex((Segment + 0.5f) / Segments, 0.f);
	}
	// The rows in between; the first and last vertex of a row share a position (the seam at the back)
	const int32 RowBase = Vertices.Num();
	for (int32 Ring = 1; Ring < Rings; ++Ring)
	{
		for (int32 Segment = 0; Segment <= Segments; ++Segment)
		{
			AddVertex(static_cast<float>(Segment) / Segments, static_cast<float>(Ring) / Rings);
		}
	}
	const int32 BottomBase = Vertices.Num();
	for (int32 Segment = 0; Segment < Segments; ++Segment)
	{
		AddVertex((Segment + 0.5f) / Segments, 1.f);
	}

	auto RowVertex = [&](int32 Ring, int32 Segment)
	{
		return RowBase + (Ring - 1) * (Segments + 1) + Segment;
	};

	using KPMeshGenPrivate::AddOutwardTriangle;
	for (int32 Segment = 0; Segment < Segments; ++Segment)
	{
		AddOutwardTriangle(Triangles, Vertices, TopBase + Segment, RowVertex(1, Segment), RowVertex(1, Segment + 1));
		for (int32 Ring = 1; Ring < Rings - 1; ++Ring)
		{
			AddOutwardTriangle(Triangles, Vertices, RowVertex(Ring, Segment), RowVertex(Ring + 1, Segment), RowVertex(Ring + 1, Segment + 1));
			AddOutwardTriangle(Triangles, Vertices, RowVertex(Ring, Segment), RowVertex(Ring + 1, Segment + 1), RowVertex(Ring, Segment + 1));
		}
		AddOutwardTriangle(Triangles, Vertices, BottomBase + Segment, RowVertex(Rings - 1, Segment + 1), RowVertex(Rings - 1, Segment));
	}

	Colors.Init(FColor::White, Vertices.Num());
	Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

int32 KPMeshGen::BuildPatchBody(UProceduralMeshComponent* Mesh, float RadiusXY, float RadiusZ, int32 PatchesU, int32 PatchesV)
{
	if (!Mesh)
	{
		return 0;
	}
	PatchesU = FMath::Clamp(PatchesU, 1, BodySegments);
	PatchesV = FMath::Clamp(PatchesV, 1, BodyRings);

	// Build the plain body once, then share its triangles out by patch
	BuildBody(Mesh, RadiusXY, RadiusZ);
	const FProcMeshSection* Whole = Mesh->GetProcMeshSection(0);
	if (!Whole)
	{
		return 0;
	}
	const TArray<FProcMeshVertex> Source = Whole->ProcVertexBuffer;
	const TArray<uint32> SourceIndices = Whole->ProcIndexBuffer;
	Mesh->ClearAllMeshSections();

	const int32 NumPatches = PatchesU * PatchesV;
	TArray<TArray<FVector>> Vertices;
	TArray<TArray<FVector>> Normals;
	TArray<TArray<FVector2D>> UVs;
	TArray<TArray<FProcMeshTangent>> Tangents;
	TArray<TArray<int32>> Triangles;
	TArray<TMap<int32, int32>> Remap;
	Vertices.SetNum(NumPatches);
	Normals.SetNum(NumPatches);
	UVs.SetNum(NumPatches);
	Tangents.SetNum(NumPatches);
	Triangles.SetNum(NumPatches);
	Remap.SetNum(NumPatches);

	for (int32 Tri = 0; Tri + 2 < SourceIndices.Num(); Tri += 3)
	{
		// Which patch: from the triangle's middle in UV space (v rows, u shifted on odd rows)
		FVector2D Mid = FVector2D::ZeroVector;
		for (int32 Corner = 0; Corner < 3; ++Corner)
		{
			Mid += Source[SourceIndices[Tri + Corner]].UV0;
		}
		Mid /= 3.0;
		const int32 Row = FMath::Clamp(FMath::FloorToInt(Mid.Y * PatchesV), 0, PatchesV - 1);
		const double Shift = (Row % 2) * 0.5 / PatchesU;
		const int32 Column = FMath::FloorToInt(FMath::Fmod(Mid.X + Shift + 1.0, 1.0) * PatchesU) % PatchesU;
		const int32 Patch = Row * PatchesU + Column;

		for (int32 Corner = 0; Corner < 3; ++Corner)
		{
			const int32 Old = static_cast<int32>(SourceIndices[Tri + Corner]);
			int32* Found = Remap[Patch].Find(Old);
			if (!Found)
			{
				const FProcMeshVertex& V = Source[Old];
				Vertices[Patch].Add(V.Position);
				Normals[Patch].Add(V.Normal);
				UVs[Patch].Add(V.UV0);
				Tangents[Patch].Add(V.Tangent);
				Found = &Remap[Patch].Add(Old, Vertices[Patch].Num() - 1);
			}
			Triangles[Patch].Add(*Found);
		}
	}

	int32 Made = 0;
	for (int32 Patch = 0; Patch < NumPatches; ++Patch)
	{
		TArray<FColor> Colors;
		Colors.Init(FColor::White, Vertices[Patch].Num());
		// Empty patches still get a section so section index = patch index
		Mesh->CreateMeshSection(Patch, Vertices[Patch], Triangles[Patch], Normals[Patch], UVs[Patch], Colors, Tangents[Patch], false);
		++Made;
	}
	return Made;
}

void KPMeshGen::BuildPieceHull(float RadiusXY, float RadiusZ, const FVector& LocalNormal, float Offset, TArray<FVector>& OutPoints)
{
	OutPoints.Reset();
	const FVector Normal = FVector(LocalNormal.X, LocalNormal.Y, 0.f).GetSafeNormal();
	const FVector Along = FVector::CrossProduct(Normal, FVector::UpVector);
	const float D = FMath::Clamp(Offset / RadiusXY, -0.95f, 0.95f); // plane position in body radii

	// The flat face: the ellipse where the plane meets the body (smaller the further it is from the centre)
	const float Shrink = FMath::Sqrt(1.f - D * D);
	constexpr int32 FacePoints = 12;
	for (int32 Index = 0; Index < FacePoints; ++Index)
	{
		const float Angle = UE_TWO_PI * Index / FacePoints;
		OutPoints.Add(Normal * (D * RadiusXY) + Along * (FMath::Cos(Angle) * RadiusXY * Shrink)
			+ FVector(0.f, 0.f, FMath::Sin(Angle) * RadiusZ * Shrink));
	}

	// The round side: a coarse sphere, keeping what is clearly beyond the face
	constexpr int32 HullSegments = 12;
	constexpr int32 HullRings = 8;
	for (int32 Ring = 1; Ring < HullRings; ++Ring)
	{
		const float Polar = UE_PI * Ring / HullRings;
		for (int32 Segment = 0; Segment < HullSegments; ++Segment)
		{
			const float Azimuth = UE_TWO_PI * Segment / HullSegments;
			const FVector Unit(FMath::Sin(Polar) * FMath::Cos(Azimuth), FMath::Sin(Polar) * FMath::Sin(Azimuth), FMath::Cos(Polar));
			if (FVector::DotProduct(Unit, Normal) > D + 0.08f)
			{
				OutPoints.Add(FVector(Unit.X * RadiusXY, Unit.Y * RadiusXY, Unit.Z * RadiusZ));
			}
		}
	}
	// And the point furthest from the face
	OutPoints.Add(Normal * RadiusXY);
}

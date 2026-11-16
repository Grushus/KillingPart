// Killing Part: shared types, tuning numbers and small helpers.

#include "KillingPart/KPTypes.h"
#include "Engine/StaticMesh.h"
#include "JsonObjectConverter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogKP);

FKPDataFile KPData::Load()
{
	FKPDataFile Data;
	const FString Path = FPaths::ProjectContentDir() / TEXT("Data/KillingPart.json");
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogKP, Warning, TEXT("Couldn't read %s, using the built-in tuning"), *Path);
	}
	else if (!FJsonObjectConverter::JsonObjectStringToUStruct(Text, &Data, 0, 0))
	{
		UE_LOG(LogKP, Error, TEXT("Content/Data/KillingPart.json isn't valid JSON, using the built-in tuning"));
		Data = FKPDataFile();
	}
	else
	{
		UE_LOG(LogKP, Log, TEXT("Loaded %s: %d plushie types"), *Path, Data.Types.Num());
	}
	if (Data.Types.Num() == 0)
	{
		Data.Types.Add(FKPPlushieType()); // a teddy, so the game still runs
	}
	if (Data.Nights.Num() == 0)
	{
		// No nights in the file: one night of five waves from the toy's formula
		FKPNight& Night = Data.Nights.AddDefaulted_GetRef();
		for (int32 Level = 1; Level <= 5; ++Level)
		{
			FKPWave& Wave = Night.Waves.AddDefaulted_GetRef();
			Wave.Count = Data.Tuning.WaveSizeBase + Data.Tuning.WaveSizePerWave * Level;
			Wave.Speed = Data.Tuning.PlushieSpeedBase + Data.Tuning.PlushieSpeedPerWave * Level;
			Wave.Interval = Data.Tuning.SpawnInterval;
		}
	}
	for (FKPNight& Night : Data.Nights)
	{
		if (Night.Waves.Num() == 0)
		{
			Night.Waves.AddDefaulted();
		}
	}
	return Data;
}

namespace KPSettingsPrivate
{
	static bool bTheatricalCut = false;
}

bool KPSettings::IsTheatrical()
{
	return KPSettingsPrivate::bTheatricalCut;
}

void KPSettings::SetTheatrical(bool bOn)
{
	KPSettingsPrivate::bTheatricalCut = bOn;
}

namespace KPVisualsPrivate
{
	UStaticMesh* LoadMesh(const TCHAR* Path)
	{
		return LoadObject<UStaticMesh>(nullptr, Path);
	}

	UMaterialInterface* LitBase()
	{
		// The basic shapes carry DefaultMaterial, which has no Color parameter (see CLAUDE.md)
		return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	UMaterialInterface* UnlitBase()
	{
		static bool bWarned = false;
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/KillingPart/Materials/M_KPUnlit.M_KPUnlit"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Material && !bWarned)
		{
			bWarned = true;
			UE_LOG(LogKP, Warning, TEXT("M_KPUnlit is missing, chalk and markers fall back to a lit material"));
		}
		return Material ? Material : LitBase();
	}
}

UStaticMesh* KPVisuals::SphereMesh()
{
	return KPVisualsPrivate::LoadMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
}

UStaticMesh* KPVisuals::CubeMesh()
{
	return KPVisualsPrivate::LoadMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
}

UStaticMesh* KPVisuals::CylinderMesh()
{
	return KPVisualsPrivate::LoadMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
}

UMaterialInstanceDynamic* KPVisuals::Lit(UObject* Outer, const FLinearColor& Color)
{
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(KPVisualsPrivate::LitBase(), Outer);
	SetColor(Material, Color);
	return Material;
}

float KPVisuals::PlushSelfLight = -1.f;
float KPVisuals::TheatricalStuffGlow = -1.f;
float KPVisuals::HeroSelfLight = 0.22f;
float KPVisuals::HeroHeight = 240.f;

UMaterialInstanceDynamic* KPVisuals::Plush(UObject* Outer, const FLinearColor& Color)
{
	static bool bWarned = false;
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/KillingPart/Materials/M_KPPlush.M_KPPlush"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Base)
	{
		if (!bWarned)
		{
			bWarned = true;
			UE_LOG(LogKP, Warning, TEXT("M_KPPlush is missing, plushies fall back to a plain lit material"));
		}
		return Lit(Outer, Color);
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Outer);
	SetColor(Material, Color);
	if (PlushSelfLight >= 0.f)
	{
		Material->SetScalarParameterValue(TEXT("SelfLight"), PlushSelfLight);
	}
	return Material;
}

UMaterialInstanceDynamic* KPVisuals::Unlit(UObject* Outer, const FLinearColor& Color)
{
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(KPVisualsPrivate::UnlitBase(), Outer);
	SetColor(Material, Color);
	return Material;
}

void KPVisuals::SetColor(UMaterialInstanceDynamic* Material, const FLinearColor& Color)
{
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

FLinearColor KPVisuals::Hex(const TCHAR* Text)
{
	return FLinearColor::FromSRGBColor(FColor::FromHex(Text));
}

FVector KPMath::CutNormal(float YawDegrees)
{
	const float YawRad = FMath::DegreesToRadians(YawDegrees);
	return FVector::CrossProduct(FVector(FMath::Cos(YawRad), FMath::Sin(YawRad), 0.f), FVector::UpVector);
}

FVector KPMath::ClosestPointOnSegment2D(const FVector& P, const FVector& A, const FVector& B)
{
	const FVector2D AB(B.X - A.X, B.Y - A.Y);
	const double LengthSq = AB.SizeSquared();
	double T = LengthSq > 0.0 ? FVector2D::DotProduct(FVector2D(P.X - A.X, P.Y - A.Y), AB) / LengthSq : 0.0;
	T = FMath::Clamp(T, 0.0, 1.0);
	return FVector(A.X + AB.X * T, A.Y + AB.Y * T, A.Z);
}

float KPMath::SegmentDistance2D(const FVector& P, const FVector& A, const FVector& B)
{
	const FVector2D P2(P.X, P.Y);
	const FVector2D A2(A.X, A.Y);
	const FVector2D AB(B.X - A.X, B.Y - A.Y);
	const double LengthSq = AB.SizeSquared();
	double T = LengthSq > 0.0 ? FVector2D::DotProduct(P2 - A2, AB) / LengthSq : 0.0;
	T = FMath::Clamp(T, 0.0, 1.0);
	return static_cast<float>(FVector2D::Distance(P2, A2 + AB * T));
}

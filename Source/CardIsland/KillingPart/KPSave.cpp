// Killing Part: the save game (T5, cut down in T5b).

#include "KillingPart/KPSave.h"
#include "KillingPart/KPTypes.h"
#include "Kismet/GameplayStatics.h"

namespace KPSavePrivate
{
	const TCHAR* const SlotName = TEXT("KillingPart");
	constexpr int32 UserIndex = 0;
}

void UKPSaveGame::FitNights(int32 NumNights)
{
	NightBest.SetNumZeroed(NumNights);
	NightBestCombo.SetNumZeroed(NumNights);
	NightWon.SetNumZeroed(NumNights);
	CurrentNight = FMath::Clamp(CurrentNight, 0, FMath::Max(0, NumNights - 1));
}

int32 UKPSaveGame::WeekScore() const
{
	int32 Total = 0;
	for (const int32 Each : NightBest)
	{
		Total += Each;
	}
	return Total;
}

UKPSaveGame* KPSave::LoadOrCreate()
{
	using namespace KPSavePrivate;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		if (UKPSaveGame* Loaded = Cast<UKPSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex)))
		{
			UE_LOG(LogKP, Log, TEXT("Save loaded: night %d, week score %d"), Loaded->CurrentNight + 1, Loaded->WeekScore());
			return Loaded;
		}
		UE_LOG(LogKP, Warning, TEXT("The save couldn't be read; starting fresh"));
	}
	return Cast<UKPSaveGame>(UGameplayStatics::CreateSaveGameObject(UKPSaveGame::StaticClass()));
}

bool KPSave::Write(UKPSaveGame* Save)
{
	if (!Save)
	{
		return false;
	}
	const bool bOk = UGameplayStatics::SaveGameToSlot(Save, KPSavePrivate::SlotName, KPSavePrivate::UserIndex);
	UE_LOG(LogKP, Log, TEXT("Saved: next night %d, week score %d%s"), Save->CurrentNight + 1, Save->WeekScore(), bOk ? TEXT("") : TEXT(" (FAILED)"));
	return bOk;
}

UKPSaveGame* KPSave::Reset()
{
	using namespace KPSavePrivate;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
	}
	UE_LOG(LogKP, Log, TEXT("Save reset"));
	return Cast<UKPSaveGame>(UGameplayStatics::CreateSaveGameObject(UKPSaveGame::StaticClass()));
}

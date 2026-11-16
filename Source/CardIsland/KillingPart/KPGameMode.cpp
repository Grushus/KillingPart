// Killing Part: the game mode.

#include "KillingPart/KPGameMode.h"
#include "KillingPart/KPDirector.h"
#include "KillingPart/KPHeroine.h"
#include "KillingPart/KPPlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AKPGameMode::AKPGameMode()
{
	DefaultPawnClass = AKPHeroine::StaticClass();
	PlayerControllerClass = AKPPlayerController::StaticClass();
}

void AKPGameMode::StartPlay()
{
	Super::StartPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (TActorIterator<AKPDirector>(World))
	{
		return; // one already placed in the level
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<AKPDirector>(AKPDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
}

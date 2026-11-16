// Killing Part: the game mode. Spawns her (the default pawn) and the director that runs the game.
// DefaultEngine.ini makes it the global default game mode, so Lvl_Mat needs no override.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KPGameMode.generated.h"

UCLASS()
class AKPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AKPGameMode();

	virtual void StartPlay() override;
};

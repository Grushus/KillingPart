// Killing Part: shows the cursor and turns it into a point on the mat. The director polls it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KPPlayerController.generated.h"

UCLASS()
class AKPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AKPPlayerController();

	virtual void BeginPlay() override;

	/** Where the cursor ray meets the plane Z = PlaneZ. False if the cursor isn't over the viewport. */
	bool GetCursorOnPlane(float PlaneZ, FVector& OutPoint) const;

	/** True while the left mouse button is held */
	bool IsCutHeld() const;
};

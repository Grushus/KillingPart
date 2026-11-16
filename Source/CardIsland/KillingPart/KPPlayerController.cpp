// Killing Part: shows the cursor and turns it into a point on the mat.

#include "KillingPart/KPPlayerController.h"
#include "InputCoreTypes.h"

AKPPlayerController::AKPPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	bEnableClickEvents = false;
	bEnableTouchEvents = false;
	// The director points the view at the fixed camera; don't follow the pawn
	bAutoManageActiveCameraTarget = false;
}

void AKPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
}

bool AKPPlayerController::GetCursorOnPlane(float PlaneZ, FVector& OutPoint) const
{
	FVector Origin;
	FVector Direction;
	if (!DeprojectMousePositionToWorld(Origin, Direction) || FMath::IsNearlyZero(Direction.Z))
	{
		return false;
	}
	const double T = (PlaneZ - Origin.Z) / Direction.Z;
	if (T <= 0.0)
	{
		return false;
	}
	OutPoint = Origin + Direction * T;
	OutPoint.Z = PlaneZ;
	return true;
}

bool AKPPlayerController::IsCutHeld() const
{
	return IsInputKeyDown(EKeys::LeftMouseButton);
}

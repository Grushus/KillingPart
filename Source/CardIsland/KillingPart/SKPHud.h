// Killing Part: the HUD, painted directly in Slate (no widget assets). Hearts, the chalk bar,
// score, wave, the xN counter at the end of the line, floating combo text, the slow-motion tint,
// hit flashes and the game over panel. Clicks pass through to the game.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/SlateBrush.h"

class AKPDirector;

class SKPHud : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SKPHud) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AKPDirector>, Director)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(64.0, 64.0); }

	/** The clickable thing (the results screen's button) at At (0-1 of the screen), from the last paint; "" for none */
	FString ActionAt(const FVector2D& At01) const;

	/** One clickable area painted this frame (0-1 of the screen) */
	struct FHit
	{
		FBox2D Rect;
		FString Action;
	};

private:
	/** Everything but the sound test (T7: OnPaint paints that on top) */
	int32 PaintGame(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

	TWeakObjectPtr<AKPDirector> Director;
	mutable TArray<FHit> Hits;
	mutable FSlateBrush PortraitBrush;   // T6: the new plushie's portrait (a render target)
};

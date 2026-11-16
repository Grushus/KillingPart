// Killing Part: the HUD, painted directly in Slate. Layout and sizes follow the toy
// (Prototype/KillingPart_Toy.html), scaled by the viewport width.

#include "KillingPart/SKPHud.h"
#include "KillingPart/KPDirector.h"
#include "KillingPart/KPPlayerController.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Fonts/CompositeFont.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace KPHudDraw
{
	const FSlateBrush* Square()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return &Brush;
	}

	const FSlateBrush* CardBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White, 14.f);
		return &Brush;
	}

	const FSlateBrush* Round()
	{
		// Default outline settings round by half the height, so a square box draws a circle
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White);
		return &Brush;
	}

	const FLinearColor Ink(FColor(35, 20, 45));
	const FLinearColor HeartOn(FColor(255, 79, 143));
	const FLinearColor Gold(FColor(255, 225, 77));
	const FLinearColor CardPaper(FColor(255, 246, 232));
	const FLinearColor BoonColor(FColor(36, 150, 104));
	const FLinearColor ThreadColor(FColor(214, 58, 104));
	const FLinearColor Muted(FColor(140, 120, 135));

	FLinearColor WithAlpha(FLinearColor Color, float Alpha)
	{
		Color.A *= Alpha;
		return Color;
	}

	/** T7b: the headings' and buttons' rounded font (Fredoka SemiBold, OFL; Content/Data/Fonts, staged with the
	 * JSON). Falls back to the default bold if the file is missing. */
	FSlateFontInfo HeadingFont(int32 Size)
	{
		static TSharedPtr<const FCompositeFont> Font;
		static bool bTried = false;
		if (!bTried)
		{
			bTried = true;
			const FString Path = FPaths::ProjectContentDir() / TEXT("Data/Fonts/Fredoka-SemiBold.ttf");
			if (IFileManager::Get().FileExists(*Path))
			{
				Font = MakeShared<FStandaloneCompositeFont>(FName(TEXT("Regular")), Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
			}
			else
			{
				UE_LOG(LogKP, Warning, TEXT("Heading font %s is missing; using the default"), *Path);
			}
		}
		if (Font.IsValid())
		{
			return FSlateFontInfo(Font, static_cast<float>(Size), FName(TEXT("Regular")));
		}
		return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
	}

	struct FHudPainter
	{
		const FGeometry& Geometry;
		FSlateWindowElementList& Out;
		int32 Layer;
		float Scale; // toy pixels to Slate units
		TArray<SKPHud::FHit>* HitList = nullptr;
		FVector2D Mouse = FVector2D(-1.0, -1.0); // the cursor in local units
		FVector2D Local = FVector2D(1.0, 1.0);   // the whole HUD's size
		bool bHeading = false;                   // T7b: Plain uses the heading font (buttons)

		/** A clickable area (local units), and whether the cursor is over it */
		bool Hit(const FVector2D& Pos, const FVector2D& Size, const FString& Action) const
		{
			if (HitList && !Action.IsEmpty() && Local.X > 0.0 && Local.Y > 0.0)
			{
				HitList->Add({ FBox2D(Pos / Local, (Pos + Size) / Local), Action });
			}
			return IsOver(Pos, Size);
		}

		bool IsOver(const FVector2D& Pos, const FVector2D& Size) const
		{
			return Mouse.X >= Pos.X && Mouse.Y >= Pos.Y && Mouse.X <= Pos.X + Size.X && Mouse.Y <= Pos.Y + Size.Y;
		}

		/** A pill (rounded by half its height) */
		void Pill(const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Color) const
		{
			FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Pos))), Round(), ESlateDrawEffect::None, Color);
		}

		/** Plain text (no outline) on one line, vertically centred on Pos. Align 0 = left, 0.5 = centre, 1 = right. Returns its width. */
		float Plain(const FString& Text, const FVector2D& Pos, float SizePx, const FLinearColor& Color, float Align = 0.5f, bool bBold = true) const
		{
			if (Text.IsEmpty())
			{
				return 0.f;
			}
			const int32 FontSize = FMath::Max(1, FMath::RoundToInt(SizePx * Scale * 0.75f));
			const FSlateFontInfo Font = bHeading ? HeadingFont(FontSize) : FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), FontSize);
			const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const FVector2f Measured = Measure->Measure(Text, Font);
			const FVector2D TopLeft(Pos.X - Measured.X * Align, Pos.Y - Measured.Y * 0.5f);
			FSlateDrawElement::MakeText(Out, Layer, Geometry.ToPaintGeometry(Measured, FSlateLayoutTransform(FVector2f(TopLeft))), Text, Font, ESlateDrawEffect::None, Color);
			return Measured.X;
		}


		void Box(const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Color) const
		{
			FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Pos))), Square(), ESlateDrawEffect::None, Color);
		}

		void Circle(const FVector2D& Center, float Radius, const FLinearColor& Color) const
		{
			const FVector2D Size(Radius * 2.f, Radius * 2.f);
			FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Center - Size * 0.5))), Round(), ESlateDrawEffect::None, Color);
		}

		void Diamond(const FVector2D& Center, float Side, const FLinearColor& Color) const
		{
			const FVector2D Size(Side, Side);
			FSlateDrawElement::MakeRotatedBox(Out, Layer, Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Center - Size * 0.5))), Square(),
				ESlateDrawEffect::None, UE_PI * 0.25f, TOptional<FVector2f>(), FSlateDrawElement::RelativeToElement, Color);
		}

		/** Toy-style label: rounded outline, vertically centred on Pos. Align 0 = left, 0.5 = centre, 1 = right. */
		void Label(const FString& Text, const FVector2D& Pos, float SizePx, const FLinearColor& Color, float Align = 0.5f, float Alpha = 1.f) const
		{
			if (Text.IsEmpty() || Alpha <= 0.f)
			{
				return;
			}
			const float Px = SizePx * Scale;
			FSlateFontInfo Font = HeadingFont(FMath::Max(1, FMath::RoundToInt(Px * 0.75f))); // T7b: the rounded heading font
			Font.OutlineSettings.OutlineSize = FMath::Max(1, FMath::RoundToInt(FMath::Max(3.f, Px / 5.f) * 0.5f));
			Font.OutlineSettings.OutlineColor = WithAlpha(FLinearColor(Ink.R, Ink.G, Ink.B, 0.7f), Alpha);
			const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const FVector2f Measured = Measure->Measure(Text, Font);
			const FVector2D Size(Measured.X, Measured.Y);
			const FVector2D TopLeft(Pos.X - Size.X * Align, Pos.Y - Size.Y * 0.5);
			FSlateDrawElement::MakeText(Out, Layer, Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(TopLeft))), Text, Font,
				ESlateDrawEffect::None, WithAlpha(Color, Alpha));
		}

		void Card(const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Color) const
		{
			FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Pos))), CardBrush(), ESlateDrawEffect::None, Color);
		}

		/** T7b: a paper card: a soft shadow under it, the card, and a stitched (dashed) seam just inside its edge */
		void PaperCard(const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Color) const
		{
			const float S = Scale;
			const int32 L = Layer;
			for (int32 Ring = 0; Ring < 3; ++Ring)
			{
				// (three widening, fainter boxes make the shadow soft)
				const float Grow = (2.f + Ring * 3.f) * S;
				FSlateDrawElement::MakeBox(Out, L, Geometry.ToPaintGeometry(FVector2f(Size + FVector2D(Grow * 2.f, Grow * 2.f)),
					FSlateLayoutTransform(FVector2f(Pos + FVector2D(-Grow, -Grow + 6.f * S)))), CardBrush(), ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.13f * Color.A));
			}
			FSlateDrawElement::MakeBox(Out, L, Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Pos))), CardBrush(), ESlateDrawEffect::None, Color);
			// The seam: short dashes in a darker shade of the card, 7 units in
			const FLinearColor Thread = FLinearColor(Color.R * 0.55f, Color.G * 0.45f, Color.B * 0.5f, 0.55f * Color.A);
			const float Inset = 7.f * S;
			const float Dash = 7.f * S;
			const float Gap = 5.f * S;
			const float Thick = FMath::Max(1.f, 1.6f * S);
			const FVector2D A = Pos + FVector2D(Inset + 6.f * S, Inset);
			const FVector2D B = Pos + Size - FVector2D(Inset + 6.f * S, Inset);
			// (a layer above the card: a different brush in the same layer may draw under it)
			auto Dashes = [&](const FVector2D& From, const FVector2D& Dir, float Length)
			{
				for (float At = 0.f; At + Dash <= Length; At += Dash + Gap)
				{
					const FVector2D P = From + Dir * At;
					const FVector2D Sz = Dir.X != 0.0 ? FVector2D(Dash, Thick) : FVector2D(Thick, Dash);
					FSlateDrawElement::MakeBox(Out, L + 1, Geometry.ToPaintGeometry(FVector2f(Sz), FSlateLayoutTransform(FVector2f(P))), Square(), ESlateDrawEffect::None, Thread);
				}
			};
			const FVector2D TopLeft(A.X, Pos.Y + Inset);
			Dashes(TopLeft, FVector2D(1.0, 0.0), B.X - A.X);
			Dashes(FVector2D(A.X, Pos.Y + Size.Y - Inset - Thick), FVector2D(1.0, 0.0), B.X - A.X);
			Dashes(FVector2D(Pos.X + Inset, Pos.Y + Inset + 6.f * S), FVector2D(0.0, 1.0), Size.Y - 2.f * (Inset + 6.f * S));
			Dashes(FVector2D(Pos.X + Size.X - Inset - Thick, Pos.Y + Inset + 6.f * S), FVector2D(0.0, 1.0), Size.Y - 2.f * (Inset + 6.f * S));
		}

		/** Plain text (no outline) wrapped to MaxWidth, centred on CenterX from Top down. Returns the height used. */
		float Wrapped(const FString& Text, float CenterX, float Top, float MaxWidth, float SizePx, const FLinearColor& Color, bool bBold, float Align = 0.5f) const
		{
			if (Text.IsEmpty())
			{
				return 0.f;
			}
			const int32 FontSize = FMath::Max(1, FMath::RoundToInt(SizePx * Scale * 0.75f));
			const FSlateFontInfo Font = bHeading ? HeadingFont(FontSize) : FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), FontSize);
			const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			// The word {heart} draws the HUD's heart icon instead (as wide as a capital letter is tall)
			const FString HeartToken(TEXT("{heart}"));
			const float LineHeight = Measure->Measure(TEXT("H"), Font).Y;
			const float IconWidth = LineHeight * 0.8f;
			// (a lone space measures 0, so measure it between two letters)
			const float SpaceWidth = FMath::Max(Measure->Measure(TEXT("a a"), Font).X - Measure->Measure(TEXT("aa"), Font).X, LineHeight * 0.26f);
			auto WordWidth = [&](const FString& Word) -> float
			{
				return Word == HeartToken ? IconWidth : Measure->Measure(Word, Font).X;
			};
			TArray<FString> Words;
			Text.ParseIntoArray(Words, TEXT(" "));
			TArray<TArray<FString>> Lines;
			TArray<FString> Line;
			float LineWidth = 0.f;
			for (const FString& Word : Words)
			{
				const float Width = WordWidth(Word);
				const float Candidate = Line.Num() == 0 ? Width : LineWidth + SpaceWidth + Width;
				if (Line.Num() > 0 && Candidate > MaxWidth)
				{
					Lines.Add(Line);
					Line.Reset();
					Line.Add(Word);
					LineWidth = Width;
				}
				else
				{
					Line.Add(Word);
					LineWidth = Candidate;
				}
			}
			if (Line.Num() > 0)
			{
				Lines.Add(Line);
			}
			float Y = Top;
			for (const TArray<FString>& Each : Lines)
			{
				if (!Each.Contains(HeartToken))
				{
					// No icon on this line: draw it as one text, so Slate spaces the words itself
					const FString Joined = FString::Join(Each, TEXT(" "));
					const FVector2f Measured = Measure->Measure(Joined, Font);
					FSlateDrawElement::MakeText(Out, Layer, Geometry.ToPaintGeometry(Measured, FSlateLayoutTransform(FVector2f(CenterX - Measured.X * Align, Y))), Joined, Font, ESlateDrawEffect::None, Color);
					Y += LineHeight;
					continue;
				}
				float Total = 0.f;
				for (int32 Index = 0; Index < Each.Num(); ++Index)
				{
					Total += WordWidth(Each[Index]) + (Index > 0 ? SpaceWidth : 0.f);
				}
				float X = CenterX - Total * Align;
				for (int32 Index = 0; Index < Each.Num(); ++Index)
				{
					if (Index > 0)
					{
						X += SpaceWidth;
					}
					if (Each[Index] == HeartToken)
					{
						Heart(FVector2D(X + IconWidth * 0.5f, Y + LineHeight * 0.1f), IconWidth, true);
						X += IconWidth;
						continue;
					}
					const FVector2f Measured = Measure->Measure(Each[Index], Font);
					FSlateDrawElement::MakeText(Out, Layer, Geometry.ToPaintGeometry(Measured, FSlateLayoutTransform(FVector2f(X, Y))), Each[Index], Font, ESlateDrawEffect::None, Color);
					X += Measured.X;
				}
				Y += LineHeight;
			}
			return Y - Top;
		}

		/** A heart whose top is at Top, Size wide and tall (toy units already scaled) */
		void Heart(const FVector2D& Top, float Size, bool bOn) const
		{
			const FLinearColor Color = bOn ? HeartOn : FLinearColor(1.f, 1.f, 1.f, 0.35f);
			Circle(Top + FVector2D(-0.23 * Size, 0.3 * Size), 0.27f * Size, Color);
			Circle(Top + FVector2D(0.23 * Size, 0.3 * Size), 0.27f * Size, Color);
			Diamond(Top + FVector2D(0.0, 0.5 * Size), 0.52f * Size, Color);
		}
	};

	FString Thousands(int32 Value)
	{
		return FText::AsNumber(Value).ToString();
	}

	/** T6d: play time as m:ss (h:mm:ss from an hour) */
	FString PlayTimeText(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::FloorToInt(Seconds));
		return Total >= 3600 ? FString::Printf(TEXT("%d:%02d:%02d"), Total / 3600, (Total / 60) % 60, Total % 60) : FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60);
	}

	// ------------------------------------------------------------------
	// The results screen after every night (T5, one screen since T5b): the week calendar, the night's
	// numbers and one button

	const FLinearColor Pink(FColor(255, 79, 143));
	const FLinearColor WonMint(FColor(189, 232, 201));

	/** A pink pill button with white text. Returns whether the cursor is on it. */
	bool PillButton(FHudPainter& C, const FVector2D& Pos, const FVector2D& Size, const FString& Text, const FString& Action, bool bEnabled = true)
	{
		const int32 L = C.Layer;
		const bool bOver = C.Hit(Pos, Size, bEnabled ? Action : FString()) && bEnabled;
		if (bOver)
		{
			C.Pill(Pos - FVector2D(3.f, 3.f) * C.Scale, Size + FVector2D(6.f, 6.f) * C.Scale, Gold);
		}
		C.Layer = L + 1;
		C.Pill(Pos, Size, WithAlpha(Pink, bEnabled ? 1.f : 0.45f));
		C.Layer = L + 2;
		C.bHeading = true; // T7b: buttons in the rounded heading font
		C.Plain(Text, Pos + Size * 0.5, 16.f, WithAlpha(FLinearColor::White, bEnabled ? 1.f : 0.6f), 0.5f, true);
		C.bHeading = false;
		C.Layer = L;
		return bOver;
	}

	/** The week across the top: Monday to Sunday with each night's best, won nights marked, the next one highlighted */
	void PaintCalendar(FHudPainter& C, const AKPDirector* D, float W)
	{
		const UKPSaveGame* Save = D->GetSave();
		const TArray<FKPNight>& Nights = D->GetNights();
		if (!Save || Nights.Num() == 0)
		{
			return;
		}
		const float S = C.Scale;
		const int32 Count = Nights.Num();
		const float Gap = 10.f * S;
		const float BoxW = FMath::Min(112.f * S, (844.f * S - Gap * (Count - 1)) / Count);
		const float BoxH = 62.f * S;
		const float Total = Count * BoxW + (Count - 1) * Gap;
		const float Left = W * 0.5f - Total * 0.5f;
		const float Top = 14.f * S;
		const int32 L = C.Layer;
		for (int32 Night = 0; Night < Count; ++Night)
		{
			const FVector2D Pos(Left + Night * (BoxW + Gap), Top);
			const FVector2D Size(BoxW, BoxH);
			// (each night's best, or in Hardcore this run's scores)
			const bool bWon = D->CalendarWon(Night);
			const int32 NightBest = D->CalendarScore(Night);
			const bool bNext = Night == D->CalendarNext();
			C.Layer = L;
			if (bNext)
			{
				C.Card(Pos - FVector2D(4.f, 4.f) * S, Size + FVector2D(8.f, 8.f) * S, Pink);
			}
			C.Layer = L + 1;
			C.Card(Pos, Size, bWon ? WonMint : NightBest > 0 ? CardPaper : FLinearColor(1.f, 1.f, 1.f, 0.6f));
			C.Layer = L + 2;
			C.Plain(Nights[Night].Name.Left(3), FVector2D(Pos.X + BoxW * 0.5f, Pos.Y + 15.f * S), 14.f, Ink, 0.5f, true);
			C.Plain(NightBest > 0 ? Thousands(NightBest) : FString(TEXT("-")), FVector2D(Pos.X + BoxW * 0.5f, Pos.Y + 35.f * S), 13.f, Ink, 0.5f, false);
			if (bWon)
			{
				C.Plain(D->Text(TEXT("won"), TEXT("won")), FVector2D(Pos.X + BoxW * 0.5f, Pos.Y + 52.f * S), 10.f, BoonColor, 0.5f, true);
			}
			else if (bNext)
			{
				C.Plain(D->Text(TEXT("next"), TEXT("next")), FVector2D(Pos.X + BoxW * 0.5f, Pos.Y + 52.f * S), 10.f, Pink, 0.5f, true);
			}
		}
		C.Layer = L + 3;
	}

	/** After the boss (T6): Sunday's dawn, a few quiet lines, the week, thanks and credits, three buttons */
	void PaintEnding(FHudPainter& C, const AKPDirector* D, float W, float H)
	{
		const float S = C.Scale;
		const FKPNightResult& R = D->GetResult();
		const float Cx = W * 0.5f;
		const float T = D->GetUiClock();
		auto FadeIn = [T](float Start) { return FMath::Clamp((T - Start) / 0.8f, 0.f, 1.f); };
		// T7: the stat lines land on sfx_results' beats (a quick pop in)
		auto OnBeat = [T, D](int32 Line) { return FMath::Clamp((T - D->ResultsLineTime(Line)) / 0.12f, 0.f, 1.f); };
		const int32 Base = ++C.Layer;
		// Dawn light over the quiet workroom
		C.Box(FVector2D::ZeroVector, FVector2D(W, H), FLinearColor(FColor(255, 190, 150)).CopyWithNewOpacity(0.35f + 0.25f * FadeIn(0.f)));
		C.Box(FVector2D::ZeroVector, FVector2D(W, H), FLinearColor(FColor(40, 20, 45)).CopyWithNewOpacity(0.35f));
		C.Layer = Base + 1;
		if (OnBeat(0) > 0.f)
		{
			PaintCalendar(C, D, W);
		}
		C.Layer = Base + 6;
		C.Label(D->Text(TEXT("endingTitle"), TEXT("Sunday, dawn.")), FVector2D(Cx, 118.f * S), 36.f, Gold, 0.5f, FadeIn(0.2f));
		C.Label(D->Text(TEXT("endingLine"), TEXT("The Firstborn is in pieces. The workroom is quiet.")), FVector2D(Cx, 156.f * S), 17.f, FLinearColor::White, 0.5f, FadeIn(1.2f));
		const FString ModeName = D->CurrentMode().Name;
		const FString Week = FString::Printf(TEXT("%s %s"), *D->Text(TEXT("weekScore"), TEXT("Week score")), *Thousands(R.WeekScore));
		C.Label(Week, FVector2D(Cx, 196.f * S), 22.f, Gold, 0.5f, OnBeat(0));
		const bool bNewRecord = R.WeekScore > R.PrevRecord;
		const FString RecordText = bNewRecord ? FString(TEXT("   ")) + D->Text(TEXT("newRecord"), TEXT("New high score!")) : FString();
		const FString RecordLine = FString::Printf(TEXT("%s %s%s"), *D->Text(TEXT("modeRecord"), TEXT("{mode} high score")).Replace(TEXT("{mode}"), *ModeName), *Thousands(R.Record), *RecordText);
		C.Label(RecordLine, FVector2D(Cx, 224.f * S), 13.f, bNewRecord ? Gold : FLinearColor::White, 0.5f, OnBeat(1));
		// T6d: the week's play time and the best time
		const bool bNewTime = R.PrevBestTime <= 0.f || R.Time < R.PrevBestTime;
		const FString NewTimeText = bNewTime ? FString(TEXT("   ")) + D->Text(TEXT("newBestTime"), TEXT("New best time!")) : FString();
		const FString TimeLine = FString::Printf(TEXT("%s %s    %s %s%s"), *D->Text(TEXT("time"), TEXT("Time")), *PlayTimeText(R.Time),
			*D->Text(TEXT("bestTime"), TEXT("Best time")), *PlayTimeText(R.BestTime), *NewTimeText);
		C.Label(TimeLine, FVector2D(Cx, 246.f * S), 13.f, bNewTime ? Gold : FLinearColor::White, 0.5f, OnBeat(2));

		// Thanks and credits (thanks on the last beat)
		const float CreditsAt = D->ResultsLineTime(3);
		C.Label(D->Text(TEXT("thanks"), TEXT("Thanks for playing")), FVector2D(Cx, 286.f * S), 26.f, Pink, 0.5f, OnBeat(3));
		C.Label(D->Text(TEXT("credits"), TEXT("Made by omgxdlmao & Claude for the 2026 Epic AI MegaJam")), FVector2D(Cx, 318.f * S), 13.f, FLinearColor::White, 0.5f, FadeIn(CreditsAt + 0.4f));
		C.Label(D->Text(TEXT("tools"), TEXT("Made with Unreal Engine 5.9, Claude, Diversion and PBRgen. 3D model: Tripo AI. Sounds and music: WeLoveIndies (weloveindies.com)")), FVector2D(Cx, 340.f * S), 11.f, FLinearColor::White, 0.5f, FadeIn(CreditsAt + 0.4f));

		// One button home (after the words, so the click that split the boss can't skip it)
		C.Layer = Base + 8;
		const bool bReady = T >= CreditsAt;
		const FVector2D Size(260.f * S, 46.f * S);
		if (T >= CreditsAt - 0.4f)
		{
			PillButton(C, FVector2D(Cx - Size.X * 0.5f, 380.f * S), Size, D->Text(TEXT("backHome"), TEXT("Back to home")), TEXT("next"), bReady);
		}
		C.Layer = Base + 12;
	}

	/** T7d: one line on what took the last heart, so players learn ("" when it isn't known) */
	FString LostCauseText(const AKPDirector* D)
	{
		const FString& Cause = D->GetResult().Cause;
		if (Cause == TEXT("pincushion")) { return D->Text(TEXT("lostPincushion"), TEXT("You cut through a pincushion.")); }
		if (Cause == TEXT("impostor")) { return D->Text(TEXT("lostImpostor"), TEXT("An impostor escaped. Cut walking pincushions before they reach the edge.")); }
		if (Cause == TEXT("boss")) { return D->Text(TEXT("lostBoss"), TEXT("The Firstborn got its hug.")); }
		if (Cause == TEXT("plushie")) { return D->Text(TEXT("lostPlushie"), TEXT("A plushie got its hug.")); }
		return FString();
	}

	/** T6c: an Extended cut run is over: the round reached, the score, the two records; Play again or home */
	void PaintRunOver(FHudPainter& C, const AKPDirector* D, float W, float H)
	{
		const float S = C.Scale;
		const FKPNightResult& R = D->GetResult();
		const float Cx = W * 0.5f;
		const int32 Base = ++C.Layer;
		C.Box(FVector2D::ZeroVector, FVector2D(W, H), FLinearColor(FColor(30, 15, 40)).CopyWithNewOpacity(0.6f));
		C.Layer = Base + 6;
		C.Label(D->Text(TEXT("lost"), TEXT("Hugged to bits")), FVector2D(Cx, 92.f * S), 30.f, FLinearColor::White);
		C.Label(FString::Printf(TEXT("%s %d"), *D->Text(TEXT("roundWord"), TEXT("Round")), R.Round), FVector2D(Cx, 134.f * S), 26.f, Gold);
		C.Label(D->CurrentMode().Name, FVector2D(Cx, 166.f * S), 13.f, FLinearColor(FColor(255, 110, 130)));
		C.Card(FVector2D(Cx - 220.f * S, 184.f * S), FVector2D(440.f * S, 138.f * S), WithAlpha(Ink, 0.72f));
		C.Label(LostCauseText(D), FVector2D(Cx, 336.f * S), 12.f, FLinearColor(FColor(255, 200, 210)));
		C.Layer = Base + 7;
		const float LeftX = Cx - 196.f * S;
		const float RightX = Cx + 196.f * S;
		float Y = 208.f * S;
		const float Row = 30.f * S;
		const FString NewRecord = D->Text(TEXT("newRecordShort"), TEXT("New!"));
		// T7: each row lands on a beat of sfx_results
		const float T = D->GetUiClock();
		auto OnBeat = [T, D](int32 Line) { return FMath::Clamp((T - D->ResultsLineTime(Line)) / 0.12f, 0.f, 1.f); };
		const FLinearColor White0 = WithAlpha(FLinearColor::White, OnBeat(0));
		C.Plain(D->Text(TEXT("score"), TEXT("Score")), FVector2D(LeftX, Y), 15.f, White0, 0.f);
		C.Plain(Thousands(R.Score), FVector2D(RightX, Y), 15.f, White0, 1.f);
		Y += Row;
		const bool bNewScore = R.Score > R.PrevRecord && R.Score > 0;
		const float A1 = OnBeat(1);
		C.Plain(D->Text(TEXT("record"), TEXT("High score")), FVector2D(LeftX, Y), 15.f, WithAlpha(FLinearColor::White, A1), 0.f);
		if (bNewScore)
		{
			C.Plain(NewRecord, FVector2D(LeftX + 110.f * S, Y), 12.f, WithAlpha(Gold, A1), 0.f);
		}
		C.Plain(Thousands(R.Record), FVector2D(RightX, Y), 15.f, WithAlpha(bNewScore ? Gold : FLinearColor::White, A1), 1.f);
		Y += Row;
		const bool bNewRound = R.Round > R.PrevBestRound;
		const float A2 = OnBeat(2);
		C.Plain(D->Text(TEXT("bestRound"), TEXT("Best round")), FVector2D(LeftX, Y), 15.f, WithAlpha(FLinearColor::White, A2), 0.f);
		if (bNewRound)
		{
			C.Plain(NewRecord, FVector2D(LeftX + 110.f * S, Y), 12.f, WithAlpha(Gold, A2), 0.f);
		}
		C.Plain(FString::Printf(TEXT("%d"), R.BestRound), FVector2D(RightX, Y), 15.f, WithAlpha(bNewRound ? Gold : FLinearColor::White, A2), 1.f);
		Y += Row;
		const FLinearColor White3 = WithAlpha(FLinearColor::White, OnBeat(3));
		C.Plain(D->Text(TEXT("bestCombo"), TEXT("Best combo")), FVector2D(LeftX, Y), 15.f, White3, 0.f);
		C.Plain(R.Combo > 0 ? FString::Printf(TEXT("x%d"), R.Combo) : FString(TEXT("-")), FVector2D(RightX, Y), 15.f, White3, 1.f);
		C.Layer = Base + 8;
		const bool bReady = D->GetUiClock() >= D->GetTuning().UiClickDelay;
		const FVector2D Size(230.f * S, 48.f * S);
		PillButton(C, FVector2D(Cx - Size.X - 10.f * S, 350.f * S), Size, D->NextNightLabel(), TEXT("next"), bReady);
		PillButton(C, FVector2D(Cx + 10.f * S, 350.f * S), Size, D->Text(TEXT("backHome"), TEXT("Back to home")), TEXT("title"), bReady);
		C.Layer = Base + 12;
	}

	/** After every night, won or lost: the calendar, the night's numbers and one button */
	void PaintResults(FHudPainter& C, const AKPDirector* D, float W, float H)
	{
		const float S = C.Scale;
		const FKPNightResult& R = D->GetResult();
		if (R.bEndless)
		{
			PaintRunOver(C, D, W, H);
			return;
		}
		if (R.bWon && R.bWeekDone)
		{
			PaintEnding(C, D, W, H);
			return;
		}
		const FString NightName = D->GetNights().IsValidIndex(R.Night) ? D->GetNights()[R.Night].Name : FString();
		const float Cx = W * 0.5f;
		const int32 Base = ++C.Layer;
		C.Box(FVector2D::ZeroVector, FVector2D(W, H), R.bWon ? FLinearColor(FColor(255, 196, 150)).CopyWithNewOpacity(0.5f) : FLinearColor(FColor(30, 15, 40)).CopyWithNewOpacity(0.6f));
		C.Layer = Base + 1;
		PaintCalendar(C, D, W);

		// The heading
		C.Layer = Base + 6;
		if (R.bWon && R.bWeekDone)
		{
			C.Label(D->Text(TEXT("weekComplete"), TEXT("Week complete!")), FVector2D(Cx, 122.f * S), 34.f, Gold);
			C.Label(FString::Printf(TEXT("%s %s"), *D->Text(TEXT("weekScore"), TEXT("Week score")), *Thousands(R.WeekScore)), FVector2D(Cx, 158.f * S), 18.f, FLinearColor::White);
		}
		else if (R.bWon)
		{
			C.Label(D->Text(TEXT("dawn"), TEXT("Dawn")), FVector2D(Cx, 122.f * S), 34.f, Gold);
			C.Label(D->Text(TEXT("survived"), TEXT("{night} survived")).Replace(TEXT("{night}"), *NightName), FVector2D(Cx, 158.f * S), 17.f, FLinearColor::White);
		}
		else
		{
			C.Label(D->Text(TEXT("lost"), TEXT("Hugged to bits")), FVector2D(Cx, 112.f * S), 30.f, FLinearColor::White);
			C.Label(NightName, FVector2D(Cx, 142.f * S), 17.f, Gold);
			C.Label(LostCauseText(D), FVector2D(Cx, 166.f * S), 13.f, FLinearColor(FColor(255, 200, 210))); // T7d: what took the last heart
		}

		// The numbers
		C.Layer = Base + 6;
		C.Card(FVector2D(Cx - 220.f * S, 180.f * S), FVector2D(440.f * S, 166.f * S), WithAlpha(Ink, 0.72f));
		C.Layer = Base + 7;
		const float LeftX = Cx - 196.f * S;
		const float RightX = Cx + 196.f * S;
		float Y = 203.f * S;
		const float Row = 30.f * S;
		C.Plain(D->Text(TEXT("score"), TEXT("Score")), FVector2D(LeftX, Y), 15.f, FLinearColor::White, 0.f);
		C.Plain(Thousands(R.Score), FVector2D(RightX, Y), 15.f, FLinearColor::White, 1.f);
		Y += Row;
		C.Plain(D->Text(TEXT("weekSoFar"), TEXT("Week score")), FVector2D(LeftX, Y), 15.f, FLinearColor::White, 0.f);
		C.Plain(Thousands(R.WeekScore), FVector2D(RightX, Y), 15.f, FLinearColor::White, 1.f);
		Y += Row;
		// T6d: the week's active play time so far
		C.Plain(D->Text(TEXT("weekTime"), TEXT("Week time")), FVector2D(LeftX, Y), 15.f, FLinearColor::White, 0.f);
		C.Plain(PlayTimeText(R.Time), FVector2D(RightX, Y), 15.f, FLinearColor::White, 1.f);
		Y += Row;
		const FString ModeName = D->CurrentMode().Name;
		C.Plain(D->Text(TEXT("modeRecord"), TEXT("{mode} high score")).Replace(TEXT("{mode}"), *ModeName), FVector2D(LeftX, Y), 15.f, FLinearColor::White, 0.f);
		C.Plain(R.Record > 0 ? Thousands(R.Record) : FString(TEXT("-")), FVector2D(RightX, Y), 15.f, FLinearColor::White, 1.f);
		Y += Row;
		C.Plain(D->Text(TEXT("bestCombo"), TEXT("Best combo")), FVector2D(LeftX, Y), 15.f, FLinearColor::White, 0.f);
		static const TCHAR* const ComboWords[] = { TEXT(""), TEXT("Snip"), TEXT("Double"), TEXT("Tailored"), TEXT("Bespoke"), TEXT("Killing part") };
		const FString Combo = R.Combo > 0 ? FString::Printf(TEXT("x%d  %s"), R.Combo, ComboWords[FMath::Min(R.Combo, 5)]) : FString(TEXT("-"));
		C.Plain(Combo, FVector2D(RightX, Y), 15.f, R.Combo >= 5 ? Gold : FLinearColor::White, 1.f);
		if (D->GetModeIndex() > 0)
		{
			C.Layer = Base + 9;
			C.Label(ModeName, FVector2D(Cx, 476.f * S), 13.f, FLinearColor(FColor(255, 110, 130)));
		}

		// The buttons (they take clicks after a moment, so the click that ended the night can't skip them);
		// T7b: Save & quit to home under the main one
		C.Layer = Base + 8;
		const bool bReady = D->GetUiClock() >= D->GetTuning().UiClickDelay;
		const FVector2D ButtonSize(280.f * S, 48.f * S);
		PillButton(C, FVector2D(Cx - ButtonSize.X * 0.5f, 366.f * S), ButtonSize, D->NextNightLabel(), TEXT("next"), bReady);
		const FVector2D HomeSize(240.f * S, 38.f * S);
		PillButton(C, FVector2D(Cx - HomeSize.X * 0.5f, 424.f * S), HomeSize, D->Text(TEXT("saveQuitHome"), TEXT("Save & quit to home")), TEXT("title"), bReady);
		C.Layer = Base + 12;
	}
}

namespace KPHudDraw
{
	/** T7d: "Camera cuts: On" / "Camera cuts: Off" */
	FString CameraCutsLabel(const AKPDirector* D)
	{
		return D->AreCameraCutsOn() ? D->Text(TEXT("cameraCutsOn"), TEXT("Camera cuts: On")) : D->Text(TEXT("cameraCutsOff"), TEXT("Camera cuts: Off"));
	}

	/** T7e: what camera cuts do, small under their button */
	void CameraCutsNote(FHudPainter& C, const AKPDirector* D, float Cx, float Y)
	{
		C.Label(D->Text(TEXT("cameraCutsNote"), TEXT("Adds a camera effect for combos of 5+")), FVector2D(Cx, Y), 10.f, FLinearColor::White.CopyWithNewOpacity(0.75f));
	}

	/** T7: a volume setting (title and pause): its name, < 80% > in steps of 10% (Key: "music" or "sfx") */
	void VolumeRow(FHudPainter& C, const FVector2D& Pos, float Width, const FString& Name, int32 Value, const TCHAR* Key, bool bReady)
	{
		const float S = C.Scale;
		const float RowH = 36.f * S;
		const int32 L = C.Layer;
		C.Pill(Pos, FVector2D(Width, RowH), WithAlpha(Ink, 0.72f));
		C.Layer = L + 1;
		C.Plain(Name, FVector2D(Pos.X + 20.f * S, Pos.Y + RowH * 0.5f), 15.f, FLinearColor::White, 0.f, true);
		const FVector2D Arrow(40.f * S, 28.f * S);
		const float ArrowY = Pos.Y + (RowH - Arrow.Y) * 0.5f;
		const float RightX = Pos.X + Width - 6.f * S - Arrow.X;
		const float ValueW = 64.f * S;
		const float LeftX = RightX - ValueW - Arrow.X;
		PillButton(C, FVector2D(LeftX, ArrowY), Arrow, TEXT("<"), bReady ? FString::Printf(TEXT("vol:%s:-1"), Key) : FString(), Value > 0);
		C.Layer = L + 1;
		PillButton(C, FVector2D(RightX, ArrowY), Arrow, TEXT(">"), bReady ? FString::Printf(TEXT("vol:%s:1"), Key) : FString(), Value < 10);
		C.Layer = L + 4;
		C.Plain(FString::Printf(TEXT("%d%%"), Value * 10), FVector2D(LeftX + Arrow.X + ValueW * 0.5f, Pos.Y + RowH * 0.5f), 15.f, FLinearColor::White, 0.5f, true);
		C.Layer = L;
	}

	/** T7: Music and Sounds, two rows from Top; returns the height used */
	float VolumeRows(FHudPainter& C, const AKPDirector* D, float Cx, float Top, float Width, bool bReady)
	{
		const float S = C.Scale;
		VolumeRow(C, FVector2D(Cx - Width * 0.5f, Top), Width, D->Text(TEXT("musicVolume"), TEXT("Music")), D->GetMusicVolume(), TEXT("music"), bReady);
		VolumeRow(C, FVector2D(Cx - Width * 0.5f, Top + 42.f * S), Width, D->Text(TEXT("sfxVolume"), TEXT("Sounds")), D->GetSfxVolume(), TEXT("sfx"), bReady);
		return 84.f * S;
	}

	/** T7: the sound test (kp.SoundTest, F8), Simon's panel for trying the options: a row per role, its option
	 *  (click: the next one, heard at once) and a play button. Over everything, at the right. */
	void PaintSoundTest(FHudPainter& C, const AKPDirector* D, float W, float H)
	{
		const float S = C.Scale;
		TArray<FString> Roles;
		D->SoundTestRoles(Roles);
		const float RowH = 25.f * S;
		const float PanelW = 400.f * S;
		const float Left = W - PanelW - 12.f * S;
		const float Top = 54.f * S;
		const float PanelH = 46.f * S + Roles.Num() * RowH + 30.f * S;
		const int32 Base = C.Layer + 1;
		C.Layer = Base;
		C.Card(FVector2D(Left, Top), FVector2D(PanelW, PanelH), WithAlpha(Ink, 0.92f));
		C.Layer = Base + 1;
		C.Plain(TEXT("Sound test"), FVector2D(Left + 14.f * S, Top + 18.f * S), 16.f, Gold, 0.f, true);
		C.Plain(TEXT("click an option: next one + hear it.  picks saved locally"), FVector2D(Left + 14.f * S, Top + 36.f * S), 10.f, Muted, 0.f, false);
		C.Layer = Base + 1;
		PillButton(C, FVector2D(Left + PanelW - 40.f * S, Top + 6.f * S), FVector2D(30.f * S, 24.f * S), TEXT("x"), TEXT("st:close"));
		float Y = Top + 48.f * S;
		for (const FString& Role : Roles)
		{
			const FKPSoundRole* Info = D->FindSoundRole(Role);
			C.Layer = Base + 1;
			const FString Name = Info && !Info->Label.IsEmpty() ? Info->Label : Role;
			C.Plain(Name, FVector2D(Left + 14.f * S, Y + RowH * 0.5f), 12.f, FLinearColor::White, 0.f, false);
			if (!Info || Info->Options.Num() == 0)
			{
				C.Plain(TEXT("(not in the JSON)"), FVector2D(Left + 150.f * S, Y + RowH * 0.5f), 11.f, Muted, 0.f, false);
				Y += RowH;
				continue;
			}
			const int32 Pick = D->SoundPickIndex(Role);
			const FString Letter = FString::Chr(static_cast<TCHAR>(TEXT('a') + Pick));
			const bool bLocal = D->GetSave() && D->GetSave()->SoundPicks.Contains(Role) && !Info->Pick.Equals(Letter, ESearchCase::IgnoreCase);
			const FString OptionText = FString::Printf(TEXT("%s/%d  %s"), *Letter, Info->Options.Num(), *Info->Options[Pick]);
			const FVector2D OptPos(Left + 160.f * S, Y + 2.f * S);
			const FVector2D OptSize(170.f * S, RowH - 4.f * S);
			const bool bOver = C.Hit(OptPos, OptSize, Info->Options.Num() > 1 ? FString::Printf(TEXT("st:cycle:%s"), *Role) : FString());
			C.Layer = Base + 1;
			C.Pill(OptPos, OptSize, bOver && Info->Options.Num() > 1 ? Pink : FLinearColor(1.f, 1.f, 1.f, 0.14f));
			C.Layer = Base + 2;
			// (gold: a local pick that differs from the JSON's default)
			C.Plain(OptionText, FVector2D(OptPos.X + 10.f * S, Y + RowH * 0.5f), 11.f, bLocal ? Gold : FLinearColor::White, 0.f, false);
			const bool bMusic = Role.StartsWith(TEXT("music"));
			const bool bPlaying = bMusic && D->GetMusicPreview() == Role;
			C.Layer = Base + 1;
			PillButton(C, FVector2D(Left + PanelW - 62.f * S, Y + 2.f * S), FVector2D(50.f * S, RowH - 4.f * S), bPlaying ? TEXT("stop") : TEXT("play"), FString::Printf(TEXT("st:play:%s"), *Role));
			Y += RowH;
		}
		C.Layer = Base + 3;
		C.Plain(TEXT("F8 or kp.SoundTest to close"), FVector2D(Left + 14.f * S, Y + 14.f * S), 10.f, Muted, 0.f, false);
		C.Layer = Base + 1;
		PillButton(C, FVector2D(Left + PanelW - 130.f * S, Y + 3.f * S), FVector2D(118.f * S, 22.f * S), TEXT("reset picks"), TEXT("st:reset"));
		C.Layer = Base + 6;
	}

	// ------------------------------------------------------------------
	// The title screen (T6): the name, the menu, the first launch's cut choice

	/** A big choice card: a title and a line under it. Returns whether the cursor is on it. */
	bool ChoiceCard(FHudPainter& C, const FVector2D& Pos, const FVector2D& Size, const FString& Title, const FString& Sub, const FString& Action, const FLinearColor& Accent)
	{
		const float S = C.Scale;
		const int32 L = C.Layer;
		const bool bOver = C.Hit(Pos, Size, Action);
		if (bOver)
		{
			C.Card(Pos - FVector2D(5.f, 5.f) * S, Size + FVector2D(10.f, 10.f) * S, Gold);
		}
		C.Layer = L + 1;
		C.PaperCard(Pos, Size, CardPaper);
		C.Layer = L + 2;
		C.Box(FVector2D(Pos.X + 24.f * S, Pos.Y + Size.Y * 0.5f - 1.f * S), FVector2D(Size.X - 48.f * S, 2.f * S), Accent.CopyWithNewOpacity(0.6f));
		C.bHeading = true;
		C.Plain(Title, FVector2D(Pos.X + Size.X * 0.5f, Pos.Y + Size.Y * 0.3f), 26.f, Ink, 0.5f, true);
		C.bHeading = false;
		C.Wrapped(Sub, Pos.X + Size.X * 0.5f, Pos.Y + Size.Y * 0.58f, Size.X - 40.f * S, 16.f, Accent, true);
		C.Layer = L;
		return bOver;
	}

	/** T6d: one of the modes screen's two cards (Side 0 = the week modes, 1 = the endless modes): arrows through
	 *  its modes, the mode's numbers and its rules as bullets; Continue and New game under it (or why it's locked) */
	void PaintModeCard(FHudPainter& C, const AKPDirector* D, int32 Side, const FVector2D& Pos, const FVector2D& Size, bool bReady)
	{
		const float S = C.Scale;
		const int32 L = C.Layer;
		TArray<int32> List;
		D->ModesOnSide(Side, List);
		const int32 Index = D->GetShownMode(Side);
		if (!D->GetModes().IsValidIndex(Index))
		{
			return;
		}
		const FKPModeInfo& Info = D->GetModes()[Index];
		const bool bOpen = D->IsModeUnlocked(Index);
		const float Mid = Pos.X + Size.X * 0.5f;
		C.PaperCard(Pos, Size, bOpen ? CardPaper : FLinearColor(FColor(172, 162, 172)));
		C.Layer = L + 1;
		C.Plain(Side == 0 ? D->Text(TEXT("weekModes"), TEXT("The week")) : D->Text(TEXT("endlessModes"), TEXT("Endless")), FVector2D(Mid, Pos.Y + 20.f * S), 12.f, Muted, 0.5f, true);

		// The arrows and the name
		const FVector2D ArrowSize(46.f * S, 36.f * S);
		const float ArrowY = Pos.Y + 36.f * S;
		if (List.Num() > 1)
		{
			PillButton(C, FVector2D(Pos.X + 14.f * S, ArrowY), ArrowSize, TEXT("<"), bReady ? FString::Printf(TEXT("modeprev:%d"), Side) : FString());
			PillButton(C, FVector2D(Pos.X + Size.X - 14.f * S - ArrowSize.X, ArrowY), ArrowSize, TEXT(">"), bReady ? FString::Printf(TEXT("modenext:%d"), Side) : FString());
		}
		C.Layer = L + 1;
		C.bHeading = true;
		C.Plain(Info.Name, FVector2D(Mid, ArrowY + ArrowSize.Y * 0.5f), 22.f, Ink, 0.5f, true);
		C.bHeading = false;
		// A dot per mode on this side (the one shown in pink; locked ones faint)
		const float DotGap = 14.f * S;
		const float DotsLeft = Mid - (List.Num() - 1) * DotGap * 0.5f;
		for (int32 N = 0; N < List.Num(); ++N)
		{
			const bool bThis = List[N] == Index;
			C.Circle(FVector2D(DotsLeft + N * DotGap, Pos.Y + 88.f * S), (bThis ? 4.5f : 3.f) * S, bThis ? Pink : Muted.CopyWithNewOpacity(D->IsModeUnlocked(List[N]) ? 0.9f : 0.35f));
		}

		// Its numbers: the high score, and the best time (week) or the best round (endless)
		float Y = Pos.Y + 112.f * S;
		if (bOpen)
		{
			const FKPModeSave* ModeSave = D->GetModeSave(Index);
			const int32 Record = D->GetModeBest(Index);
			const FString ScoreLine = Record > 0 ? FString::Printf(TEXT("%s %s"), *D->Text(TEXT("record"), TEXT("High score")), *Thousands(Record)) : D->Text(TEXT("noRecord"), TEXT("No high score yet"));
			C.Plain(ScoreLine, FVector2D(Mid, Y), 14.f, Record > 0 ? ThreadColor : Muted, 0.5f, true);
			// Both stats always show (an empty one says so)
			FString Second;
			bool bHasSecond = false;
			if (Info.Endless)
			{
				bHasSecond = ModeSave && ModeSave->BestRound > 0;
				Second = bHasSecond ? FString::Printf(TEXT("%s %d"), *D->Text(TEXT("bestRound"), TEXT("Best round")), ModeSave->BestRound)
					: D->Text(TEXT("noBestRound"), TEXT("No best round yet"));
			}
			else
			{
				bHasSecond = ModeSave && ModeSave->BestTime > 0.f;
				Second = bHasSecond ? FString::Printf(TEXT("%s %s"), *D->Text(TEXT("bestTime"), TEXT("Best time")), *PlayTimeText(ModeSave->BestTime))
					: D->Text(TEXT("noBestTime"), TEXT("No best time yet"));
			}
			C.Plain(Second, FVector2D(Mid, Y + 21.f * S), 14.f, bHasSecond ? ThreadColor : Muted, 0.5f, true);
		}
		else
		{
			C.Plain(D->Text(TEXT("locked"), TEXT("Locked")), FVector2D(Mid, Y + 10.f * S), 16.f, Ink.CopyWithNewOpacity(0.7f), 0.5f, true);
		}

		// The rules, one bullet each
		C.Box(FVector2D(Pos.X + 20.f * S, Pos.Y + 148.f * S), FVector2D(Size.X - 40.f * S, 2.f * S), Muted.CopyWithNewOpacity(0.4f));
		Y = Pos.Y + 162.f * S;
		const FLinearColor RuleColor = bOpen ? Ink : Ink.CopyWithNewOpacity(0.75f);
		for (const FString& Bullet : Info.Rules)
		{
			C.Circle(FVector2D(Pos.X + 28.f * S, Y + 9.f * S), 3.f * S, bOpen ? Pink : RuleColor);
			Y += C.Wrapped(Bullet, Pos.X + 40.f * S, Y, Size.X - 60.f * S, 14.f, RuleColor, false, 0.f) + 7.f * S;
		}

		// Under the card: Continue (with a save) and New game, for the mode shown; or why it's locked
		C.Layer = L + 2;
		float ButtonY = Pos.Y + Size.Y + 8.f * S;
		if (!bOpen)
		{
			C.Label(Info.Locked, FVector2D(Mid, ButtonY + 18.f * S), 15.f, Gold);
			C.Layer = L;
			return;
		}
		const FVector2D ButtonSize(Size.X, 36.f * S);
		const FString ContinueText = D->ModeContinueLabel(Index);
		if (!ContinueText.IsEmpty())
		{
			PillButton(C, FVector2D(Pos.X, ButtonY), ButtonSize, ContinueText, bReady ? FString::Printf(TEXT("continue:%d"), Index) : FString());
			ButtonY += ButtonSize.Y + 8.f * S;
		}
		PillButton(C, FVector2D(Pos.X, ButtonY), ButtonSize, D->Text(TEXT("newGame"), TEXT("New game")), bReady ? FString::Printf(TEXT("new:%d"), Index) : FString());
		C.Layer = L;
	}

	/** T7d: the wardrobe: she stands on the left in the lamp light; on the right a row per part with arrows and swatches */
	void PaintWardrobe(FHudPainter& C, const AKPDirector* D, float W, float H, bool bReady)
	{
		const float S = C.Scale;
		const int32 L = C.Layer;
		PillButton(C, FVector2D(16.f * S, 14.f * S), FVector2D(96.f * S, 36.f * S), FString(TEXT("< ")) + D->Text(TEXT("back"), TEXT("Back")), bReady ? TEXT("back") : TEXT(""));
		C.Layer = L + 1;
		const float PanelW = 470.f * S;
		const float Left = W - PanelW - 30.f * S;
		const float RowH = 74.f * S;
		const int32 First = AKPDirector::FirstShownWardrobeRow; // T7f: only the shears and the trail
		const int32 Shown = AKPDirector::NumWardrobeRows - First;
		const float Top = FMath::Max(70.f * S, H * 0.5f - (Shown * RowH + 60.f * S) * 0.5f);
		C.Card(FVector2D(Left, Top), FVector2D(PanelW, Shown * RowH + 64.f * S), WithAlpha(Ink, 0.78f));
		C.Layer = L + 2;
		C.Label(D->Text(TEXT("wardrobe"), TEXT("Wardrobe")), FVector2D(Left + PanelW * 0.5f, Top + 28.f * S), 26.f, Gold);
		for (int32 Row = First; Row < AKPDirector::NumWardrobeRows; ++Row)
		{
			const float Y = Top + 56.f * S + (Row - First) * RowH;
			C.Layer = L + 2;
			C.Plain(D->WardrobeRowName(Row), FVector2D(Left + 24.f * S, Y + 14.f * S), 14.f, FLinearColor::White, 0.f, true);
			TArray<FLinearColor> Swatches;
			D->WardrobeSwatches(Row, Swatches);
			const int32 Pick = Swatches.Num() > 0 ? D->WardrobePick(Row) % Swatches.Num() : 0;
			const FVector2D Arrow(36.f * S, 30.f * S);
			const float SwatchY = Y + 46.f * S;
			PillButton(C, FVector2D(Left + 20.f * S, SwatchY - Arrow.Y * 0.5f), Arrow, TEXT("<"), bReady ? FString::Printf(TEXT("wdprev:%d"), Row) : FString());
			C.Layer = L + 2;
			PillButton(C, FVector2D(Left + PanelW - 20.f * S - Arrow.X, SwatchY - Arrow.Y * 0.5f), Arrow, TEXT(">"), bReady ? FString::Printf(TEXT("wdnext:%d"), Row) : FString());
			const float SpanLeft = Left + 20.f * S + Arrow.X + 18.f * S;
			const float SpanRight = Left + PanelW - 20.f * S - Arrow.X - 18.f * S;
			const float Step = Swatches.Num() > 1 ? (SpanRight - SpanLeft) / (Swatches.Num() - 1) : 0.f;
			for (int32 Index = 0; Index < Swatches.Num(); ++Index)
			{
				const FVector2D Center(SpanLeft + Step * Index, SwatchY);
				const float Radius = 13.f * S;
				C.Layer = L + 3;
				const bool bOver = C.Hit(Center - FVector2D(Radius, Radius) * 1.3f, FVector2D(Radius, Radius) * 2.6f, bReady ? FString::Printf(TEXT("wd:%d:%d"), Row, Index) : FString());
				if (Index == Pick || bOver)
				{
					C.Circle(Center, Radius + 4.f * S, Index == Pick ? Gold : FLinearColor::White.CopyWithNewOpacity(0.6f));
				}
				C.Layer = L + 4;
				C.Circle(Center, Radius, Swatches[Index]);
			}
		}
		C.Layer = L + 6;
	}

	void PaintTitle(FHudPainter& C, const AKPDirector* D, float W, float H)
	{
		const float S = C.Scale;
		const float Cx = W * 0.5f;
		const int32 Base = ++C.Layer;
		// A soft night wash so the words read over the mat playing itself (not in the wardrobe: she's the picture)
		if (D->GetTitlePage() != 4)
		{
			C.Box(FVector2D::ZeroVector, FVector2D(W, H), FLinearColor(FColor(20, 10, 30)).CopyWithNewOpacity(0.38f));
		}
		C.Layer = Base + 2;
		if (D->GetTitlePage() != 3 && D->GetTitlePage() != 4)
		{
			C.Label(D->Text(TEXT("title"), TEXT("Killing Part")), FVector2D(Cx, 88.f * S), 70.f, Pink);
			C.Label(D->Text(TEXT("tagline"), TEXT("")), FVector2D(Cx, 136.f * S), 15.f, FLinearColor::White); // (Simon's call: empty until chosen)
		}

		const bool bReady = D->GetUiClock() >= 0.25f;
		if (D->GetTitlePage() == 1)
		{
			// First launch: which version to watch
			C.Label(D->Text(TEXT("chooseCut"), TEXT("Which version do you want to play?")), FVector2D(Cx, 200.f * S), 22.f, FLinearColor::White);
			const FVector2D Size(320.f * S, 150.f * S);
			const float Gap = 36.f * S;
			const float Top = 238.f * S;
			C.Layer = Base + 4;
			ChoiceCard(C, FVector2D(Cx - Gap * 0.5f - Size.X, Top), Size, D->Text(TEXT("directorsCut"), TEXT("Director's cut")),
				D->Text(TEXT("directorsCutSub"), TEXT("Blood")), bReady ? TEXT("choose:director") : TEXT(""), FLinearColor(FColor(200, 24, 52)));
			ChoiceCard(C, FVector2D(Cx + Gap * 0.5f, Top), Size, D->Text(TEXT("theatricalCut"), TEXT("Theatrical cut")),
				D->Text(TEXT("theatricalCutSub"), TEXT("No blood, white stuffing")), bReady ? TEXT("choose:theatrical") : TEXT(""), BoonColor);
			C.Layer = Base + 8;
			C.Label(D->Text(TEXT("cutLater"), TEXT("You can switch any time from the menu")), FVector2D(Cx, Top + Size.Y + 30.f * S), 12.f, FLinearColor::White);
		}
		else if (D->GetTitlePage() == 3)
		{
			// T6d: Play: two tall cards, the week modes on the left and the endless modes on the right, each
			// stepping through its modes with arrows; Continue and New game under each for the mode it shows.
			// The cards shrink on short screens so the buttons stay clear of the footer.
			C.Layer = Base + 4;
			// "< Back" (wider than the cards' arrows, so it doesn't look like one of them)
			PillButton(C, FVector2D(16.f * S, 14.f * S), FVector2D(96.f * S, 36.f * S), FString(TEXT("< ")) + D->Text(TEXT("back"), TEXT("Back")), bReady ? TEXT("back") : TEXT(""));
			C.Layer = Base + 6;
			C.Label(D->Text(TEXT("chooseMode"), TEXT("Choose a mode")), FVector2D(Cx, 32.f * S), 24.f, FLinearColor::White);
			const float Top = 62.f * S;
			const float CardW = 380.f * S;
			const float CardH = FMath::Clamp(H - Top - 96.f * S - 36.f * S, 300.f * S, 345.f * S);
			const float Gap = 48.f * S;
			C.Layer = Base + 5;
			PaintModeCard(C, D, 0, FVector2D(Cx - Gap * 0.5f - CardW, Top), FVector2D(CardW, CardH), bReady);
			C.Layer = Base + 5;
			PaintModeCard(C, D, 1, FVector2D(Cx + Gap * 0.5f, Top), FVector2D(CardW, CardH), bReady);
			// T8: Practice under the cards: one button per night, Normal rules, nothing saved
			const TArray<FKPNight>& PracticeNights = D->GetNights();
			if (PracticeNights.Num() > 0)
			{
				C.Layer = Base + 6;
				const float DayGap = 8.f * S;
				const FVector2D DaySize(70.f * S, 30.f * S);
				const float RowW = PracticeNights.Num() * DaySize.X + (PracticeNights.Num() - 1) * DayGap;
				const float DayY = FMath::Max(Top + CardH + 100.f * S, H - 76.f * S);
				C.Label(D->Text(TEXT("practiceLine"), TEXT("Practice: any night, nothing is saved")), FVector2D(Cx, DayY - 13.f * S), 14.f, FLinearColor::White);
				for (int32 Night = 0; Night < PracticeNights.Num(); ++Night)
				{
					PillButton(C, FVector2D(Cx - RowW * 0.5f + Night * (DaySize.X + DayGap), DayY), DaySize, PracticeNights[Night].Name.Left(3),
						bReady ? FString::Printf(TEXT("practice:%d"), Night) : FString());
				}
			}
		}
		else if (D->GetTitlePage() == 4)
		{
			PaintWardrobe(C, D, W, H, bReady);
		}
		else
		{
			// The menu (T6c): Play, the cut, Quit (T7d: the wardrobe, camera cuts)
			const FVector2D Size(300.f * S, 44.f * S);
			const float Step = 53.f * S;
			float Y = 184.f * S;
			C.Layer = Base + 4;
			PillButton(C, FVector2D(Cx - Size.X * 0.5f, Y), Size, D->Text(TEXT("play"), TEXT("Play")), bReady ? TEXT("play") : TEXT(""));
			Y += Step;
			PillButton(C, FVector2D(Cx - Size.X * 0.5f, Y), Size, D->Text(TEXT("wardrobe"), TEXT("Wardrobe")), bReady ? TEXT("wardrobe") : TEXT(""));
			Y += Step;
			const FString CutLabel = KPSettings::IsTheatrical() ? D->Text(TEXT("cutTheatrical"), TEXT("Cut: Theatrical (no blood)")) : D->Text(TEXT("cutDirectors"), TEXT("Cut: Director's (blood)"));
			PillButton(C, FVector2D(Cx - Size.X * 0.5f, Y), Size, CutLabel, bReady ? TEXT("togglecut") : TEXT(""));
			Y += Step;
			PillButton(C, FVector2D(Cx - Size.X * 0.5f, Y), Size, CameraCutsLabel(D), bReady ? TEXT("togglecam") : TEXT(""));
			Y += Size.Y + 12.f * S;
			CameraCutsNote(C, D, Cx, Y);
			Y += Step - Size.Y + 2.f * S;
			// T7: Music and Sounds
			Y += VolumeRows(C, D, Cx, Y, Size.X, bReady) + 12.f * S;
			PillButton(C, FVector2D(Cx - Size.X * 0.5f, Y), Size, D->Text(TEXT("quit"), TEXT("Quit")), bReady ? TEXT("quit") : TEXT(""));
		}

		// The first launch of a build prepares its shaders for a while: say so, so it never looks frozen
		C.Layer = Base + 12;
		if (D->IsPreparing())
		{
			const int32 Dots = 1 + FMath::FloorToInt(D->GetRealClock() * 2.5f) % 3;
			C.Label(D->Text(TEXT("preparing"), TEXT("Preparing")) + FString::ChrN(Dots, TEXT('.')), FVector2D(W - 16.f * S, H - 18.f * S), 12.f, FLinearColor::White, 1.f);
		}
		C.Label(D->Text(TEXT("credit"), TEXT("2026 Epic AI MegaJam")), FVector2D(16.f * S, H - 18.f * S), 10.f, FLinearColor::White.CopyWithNewOpacity(0.7f), 0.f);
		// T8: the version, small
		C.Label(D->Text(TEXT("version"), TEXT("v1.1")), FVector2D(Cx, H - 18.f * S), 10.f, FLinearColor::White.CopyWithNewOpacity(0.5f));
		C.Layer = Base + 14;
	}

	/** Pause: Resume, the cut, Restart night, Title screen, Quit */
	void PaintPause(FHudPainter& C, const AKPDirector* D, float W, float H)
	{
		const float S = C.Scale;
		const float Cx = W * 0.5f;
		const int32 Base = C.Layer + 2;
		C.Layer = Base;
		C.Box(FVector2D::ZeroVector, FVector2D(W, H), FLinearColor(FColor(20, 10, 30)).CopyWithNewOpacity(0.45f));
		C.Layer = Base + 2;
		C.Label(D->Text(TEXT("paused"), TEXT("Paused")), FVector2D(Cx, 74.f * S), 40.f, Gold);
		const FVector2D Size(300.f * S, 46.f * S);
		const float Step = 52.f * S;
		float Y = 112.f * S;
		const bool bReady = D->GetUiClock() >= 0.15f;
		C.Layer = Base + 4;
		PillButton(C, FVector2D(Cx - Size.X * 0.5f, Y), Size, D->Text(TEXT("resume"), TEXT("Resume")), bReady ? TEXT("resume") : TEXT(""));
		Y += Step;
		const FString CutLabel = KPSettings::IsTheatrical() ? D->Text(TEXT("cutTheatrical"), TEXT("Cut: Theatrical (no blood)")) : D->Text(TEXT("cutDirectors"), TEXT("Cut: Director's (blood)"));
		PillButton(C, FVector2D(Cx - Size.X * 0.5f, Y), Size, CutLabel, bReady ? TEXT("togglecut") : TEXT(""));
		Y += Step;
		PillButton(C, FVector2D(Cx - Size.X * 0.5f, Y), Size, CameraCutsLabel(D), bReady ? TEXT("togglecam") : TEXT(""));
		Y += Size.Y + 12.f * S;
		CameraCutsNote(C, D, Cx, Y);
		Y += Step - Size.Y + 2.f * S;
		// T7: Music and Sounds
		Y += VolumeRows(C, D, Cx, Y, Size.X, bReady) + 8.f * S;
		const FString RestartLabel = D->IsEndless() ? D->Text(TEXT("restartRound"), TEXT("Restart round")) : D->Text(TEXT("restartNight"), TEXT("Restart night"));
		PillButton(C, FVector2D(Cx - Size.X * 0.5f, Y), Size, RestartLabel, bReady ? TEXT("restart") : TEXT(""));
		Y += Step;
		// T6d: say that progress is kept
		PillButton(C, FVector2D(Cx - Size.X * 0.5f, Y), Size, D->Text(TEXT("saveQuitHome"), TEXT("Save & quit to home")), bReady ? TEXT("title") : TEXT(""));
		Y += Step;
		PillButton(C, FVector2D(Cx - Size.X * 0.5f, Y), Size, D->Text(TEXT("saveQuitGame"), TEXT("Save & quit game")), bReady ? TEXT("quit") : TEXT(""));
		Y += Step;
		C.Layer = Base + 10;
		C.Label(D->Text(TEXT("pauseHint"), TEXT("Esc to resume")), FVector2D(Cx, Y + 4.f * S), 12.f, FLinearColor::White);
		C.Layer = Base + 12;
	}
}

FString SKPHud::ActionAt(const FVector2D& At01) const
{
	// The last one painted is on top
	for (int32 Index = Hits.Num() - 1; Index >= 0; --Index)
	{
		if (Hits[Index].Rect.IsInside(At01))
		{
			return Hits[Index].Action;
		}
	}
	return FString();
}

void SKPHud::Construct(const FArguments& InArgs)
{
	Director = InArgs._Director;
	SetVisibility(EVisibility::HitTestInvisible);
}

int32 SKPHud::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace KPHudDraw;

	Hits.Reset();
	const int32 Layer = PaintGame(AllottedGeometry, OutDrawElements, LayerId);
	const AKPDirector* D = Director.Get();
	if (!D || !D->IsSoundTestOpen())
	{
		return Layer;
	}
	// T7: the sound test, over everything (its buttons are painted last, so they win the click)
	const FVector2D Local = FVector2D(AllottedGeometry.GetLocalSize());
	FHudPainter Canvas{ AllottedGeometry, OutDrawElements, Layer + 1, static_cast<float>(Local.X / 1100.0) };
	Canvas.HitList = &Hits;
	Canvas.Local = Local;
	const AKPPlayerController* PC = D->GetPlayerController();
	int32 ViewX = 0;
	int32 ViewY = 0;
	float MouseX = 0.f;
	float MouseY = 0.f;
	if (PC)
	{
		PC->GetViewportSize(ViewX, ViewY);
	}
	if (PC && ViewX > 0 && ViewY > 0 && PC->GetMousePosition(MouseX, MouseY))
	{
		Canvas.Mouse = FVector2D(MouseX / ViewX * Local.X, MouseY / ViewY * Local.Y);
	}
	PaintSoundTest(Canvas, D, Local.X, Local.Y);
	return Canvas.Layer;
}

int32 SKPHud::PaintGame(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	using namespace KPHudDraw;

	const AKPDirector* D = Director.Get();
	if (!D)
	{
		return LayerId;
	}

	const FVector2D Local = FVector2D(AllottedGeometry.GetLocalSize());
	const float W = Local.X;
	const float H = Local.Y;
	const float S = W / 1100.f;
	FHudPainter Canvas{ AllottedGeometry, OutDrawElements, LayerId + 1, S };

	// World to HUD position
	const AKPPlayerController* PC = D->GetPlayerController();
	int32 ViewX = 0;
	int32 ViewY = 0;
	if (PC)
	{
		PC->GetViewportSize(ViewX, ViewY);
	}
	// Clickable areas (T5): recorded while painting, read back by the director's click (ActionAt)
	Canvas.HitList = &Hits;
	Canvas.Local = Local;
	float MouseX = 0.f;
	float MouseY = 0.f;
	if (PC && ViewX > 0 && ViewY > 0 && PC->GetMousePosition(MouseX, MouseY))
	{
		Canvas.Mouse = FVector2D(MouseX / ViewX * W, MouseY / ViewY * H);
	}

	// The title screen (T6): the menu over the mat playing itself, none of the night's HUD
	if (D->IsTitle())
	{
		PaintTitle(Canvas, D, W, H);
		return Canvas.Layer;
	}

	// Meet a new plushie (T6): its portrait over the blurred mat, on its own
	if (D->GetMode() == EKPMode::Intro)
	{
		const float Cx = W * 0.5f;
		Canvas.Layer++;
		Canvas.Box(FVector2D::ZeroVector, Local, FLinearColor(FColor(15, 8, 25)).CopyWithNewOpacity(0.55f));
		Canvas.Layer++;
		const FString Heading = D->IsIntroBoss() ? D->Text(TEXT("meetBoss"), TEXT("The last wave"))
			: D->IsIntroHazard() ? D->Text(TEXT("meetHazard"), TEXT("New hazard")) : D->Text(TEXT("meetNew"), TEXT("New plushie"));
		Canvas.Label(Heading, FVector2D(Cx, 42.f * S), 18.f, D->IsIntroHazard() ? FLinearColor(FColor(255, 110, 130)) : KPHudDraw::Gold);
		// The portrait in a round-cornered frame, popping in
		const float Pop = KPMath::EaseOutBack(FMath::Clamp(D->GetUiClock() / 0.35f, 0.f, 1.f));
		const float Side = 330.f * S * Pop;
		const FVector2D Pos(Cx - Side * 0.5f, 70.f * S + (330.f * S - Side) * 0.5f);
		if (UTextureRenderTarget2D* Texture = D->GetPortraitTexture())
		{
			Canvas.Card(Pos - FVector2D(6.f, 6.f) * S * Pop, FVector2D(Side, Side) + FVector2D(12.f, 12.f) * S * Pop, KPHudDraw::Gold);
			Canvas.Layer++;
			PortraitBrush.SetResourceObject(Texture);
			PortraitBrush.ImageSize = FVector2D(Texture->SizeX, Texture->SizeY);
			PortraitBrush.DrawAs = ESlateBrushDrawType::Image;
			FSlateDrawElement::MakeBox(OutDrawElements, Canvas.Layer, AllottedGeometry.ToPaintGeometry(FVector2f(Side, Side), FSlateLayoutTransform(FVector2f(Pos))), &PortraitBrush);
		}
		Canvas.Layer++;
		const float TextY = 70.f * S + 330.f * S + 40.f * S;
		Canvas.Label(D->GetIntroName(), FVector2D(Cx, TextY), 38.f, KPHudDraw::Gold);
		Canvas.Label(D->GetIntroLine(), FVector2D(Cx, TextY + 44.f * S), 19.f, FLinearColor::White);
		if (D->GetUiClock() >= D->GetTuning().UiClickDelay)
		{
			const float Blink = 0.6f + 0.4f * FMath::Sin(D->GetRealClock() * 4.f);
			Canvas.Label(D->Text(TEXT("clickContinue"), TEXT("Click to continue")), FVector2D(Cx, TextY + 84.f * S), 12.f, FLinearColor::White, 0.5f, Blink);
		}
		if (D->IsGamePaused())
		{
			PaintPause(Canvas, D, W, H);
		}
		return Canvas.Layer;
	}

	// After every night, won or lost: the results screen on its own, none of the night's HUD (T5b)
	if (D->GetMode() == EKPMode::Over || D->GetMode() == EKPMode::Dawn)
	{
		PaintResults(Canvas, D, W, H);
		return Canvas.Layer;
	}

	auto ToLocal = [&](const FVector& World, FVector2D& OutPos) -> bool
	{
		FVector2D Screen;
		if (!PC || ViewX <= 0 || ViewY <= 0 || !PC->ProjectWorldLocationToScreen(World, Screen, false))
		{
			return false;
		}
		OutPos = FVector2D(Screen.X / ViewX * W, Screen.Y / ViewY * H);
		return true;
	};

	// T7d: a camera cut: letterbox bars
	if (D->IsCineActive())
	{
		Canvas.Box(FVector2D::ZeroVector, FVector2D(W, H * 0.09f), FLinearColor::Black);
		Canvas.Box(FVector2D(0.f, H * 0.91f), FVector2D(W, H * 0.09f), FLinearColor::Black);
	}

	// Slow-motion tint and hit flash
	const float Ts = D->GetTimeScale();
	if (Ts < 0.7f)
	{
		Canvas.Box(FVector2D::ZeroVector, Local, FLinearColor(FColor(45, 25, 80)).CopyWithNewOpacity((0.7f - Ts) * 0.3f));
	}
	if (D->GetFlash() > 0.f)
	{
		Canvas.Box(FVector2D::ZeroVector, Local, D->GetFlashColor().CopyWithNewOpacity(FMath::Min(0.35f, D->GetFlash())));
	}
	Canvas.Layer++;

	// xN at the end of the line
	FVector LineEnd;
	if (D->GetPreviewCount() > 0 && D->GetLineEnd(LineEnd))
	{
		FVector2D Pos;
		if (ToLocal(LineEnd, Pos))
		{
			Canvas.Label(FString::Printf(TEXT("x%d"), D->GetPreviewCount()), Pos + FVector2D(12.0, -14.0) * S, 16.f, FLinearColor::White, 0.f);
		}
	}

	// Floating texts
	for (const FKPFloatText& Text : D->GetTexts())
	{
		FVector2D Pos;
		if (Text.bOnScreen)
		{
			Pos = FVector2D(Text.Screen.X * W, Text.Screen.Y * H);
		}
		else if (!ToLocal(Text.World, Pos))
		{
			continue;
		}
		const float Age = Text.MaxLife - Text.Life;
		const float Pop = Age < 0.12f ? 1.f + (1.f - Age / 0.12f) * 0.5f : 1.f;
		const float Alpha = FMath::Clamp(Text.Life / 0.35f, 0.f, 1.f);
		Pos.Y -= Age * 18.f * S;
		Canvas.Label(Text.Text, Pos, Text.Size * Pop, Text.Color, 0.5f, Alpha);
		if (!Text.Sub.IsEmpty())
		{
			Canvas.Label(Text.Sub, Pos + FVector2D(0.0, Text.Size * 0.9f * S), 12.f, FLinearColor::White, 0.5f, Alpha);
		}
	}
	Canvas.Layer++;

	// Hearts
	for (int32 Index = 0; Index < FMath::Max(D->GetHearts(), D->GetTuning().Hearts); ++Index)
	{
		Canvas.Heart(FVector2D((20.f + Index * 20.f) * S, 10.f * S), 15.f * S, Index < D->GetHearts());
	}

	// Chalk bar
	const FVector2D BarPos(12.f * S, 32.f * S);
	const FVector2D BarSize(70.f * S, 5.f * S);
	Canvas.Box(BarPos, BarSize, FLinearColor(1.f, 1.f, 1.f, 0.3f));
	Canvas.Box(BarPos, FVector2D(BarSize.X * D->GetChalkLeft01(), BarSize.Y), FLinearColor::White);
	Canvas.Label(TEXT("chalk"), FVector2D(88.f * S, 35.f * S), 11.f, FLinearColor::White, 0.f);


	// Score, best and wave
	Canvas.Label(FString(TEXT("Score ")) + Thousands(D->GetScore()), FVector2D(W - 12.f * S, 16.f * S), 15.f, FLinearColor::White, 1.f);
	if (D->GetBest() > 0)
	{
		// (endless: the mode's high score; a week: this night's best)
		const FString BestWord = D->IsEndless() ? D->Text(TEXT("record"), TEXT("High score")) : FString(TEXT("Best"));
		Canvas.Label(BestWord + TEXT(" ") + Thousands(D->GetBest()), FVector2D(W - 12.f * S, 36.f * S), 11.f, FLinearColor::White, 1.f);
	}
	// The night, the wave and the night's rule
	const FKPNight& Night = D->CurrentNight();
	const FString NightLine = D->IsEndless()
		// (endless: the rounds made so far, so a resumed run shows the round it is about to start again)
		? (D->GetNumWaves() > 0 ? FString::Printf(TEXT("%s   %s %d"), *Night.Name, *D->Text(TEXT("roundWord"), TEXT("Round")), FMath::Max(D->GetWave(), D->GetNumWaves())) : Night.Name)
		: D->GetWave() > 0 ? FString::Printf(TEXT("%s   wave %d of %d"), *Night.Name, D->GetWave(), D->GetNumWaves())
		: Night.Name;
	Canvas.Label(NightLine, FVector2D(W * 0.5f, 16.f * S), 13.f, FLinearColor::White);
	if (!Night.RuleName.IsEmpty())
	{
		Canvas.Label(Night.RuleName, FVector2D(W * 0.5f, 33.f * S), 10.f, Gold);
	}
	// The boss (T6): its name and a pip for every cut it can still take
	const int32 BossLeft = D->GetBossCutsLeft();
	if (BossLeft >= 0)
	{
		// T6c: its name, and a heart with the hits left counting down
		Canvas.Label(D->GetBossName(), FVector2D(W * 0.5f, 54.f * S), 14.f, Gold);
		Canvas.Heart(FVector2D(W * 0.5f - 30.f * S, 64.f * S), 16.f * S, true);
		Canvas.Label(FString::Printf(TEXT("x%d"), BossLeft), FVector2D(W * 0.5f - 8.f * S, 73.f * S), 16.f, FLinearColor::White, 0.f);
	}

	// Cards held this night, once each with its level
	TArray<int32> Shown;
	for (const int32 CardIndex : D->GetHeldCards())
	{
		Shown.AddUnique(CardIndex);
	}
	for (int32 Index = 0; Index < Shown.Num(); ++Index)
	{
		const FKPCard& Card = D->GetCards()[Shown[Index]];
		// (T7c: a curse in rose; T7e: what it does is summed up in the red column)
		Canvas.Label(KPCardText::Title(Card.Name, D->GetCardLevel(Shown[Index])), FVector2D(12.f * S, H - (14.f + 15.f * (Shown.Num() - 1 - Index)) * S), 10.f,
			Card.Curse ? FLinearColor(FColor(255, 110, 130)) : FLinearColor::White, 0.f);
	}
	// T7e: what they add up to: the boons in green in the middle, the loose threads and curses in red on the right
	if (!D->IsTitle())
	{
		TArray<FString> Boons;
		TArray<FString> Threads;
		D->GetActiveEffects(Boons, Threads);
		for (int32 Index = 0; Index < Boons.Num(); ++Index)
		{
			Canvas.Label(Boons[Index], FVector2D(W * 0.5f, H - (14.f + 15.f * (Boons.Num() - 1 - Index)) * S), 10.f, FLinearColor(FColor(130, 235, 150)));
		}
		for (int32 Index = 0; Index < Threads.Num(); ++Index)
		{
			Canvas.Label(Threads[Index], FVector2D(W - 22.f * S, H -(14.f + 15.f * (Threads.Num() - 1 - Index)) * S), 10.f, FLinearColor(FColor(255, 110, 130)), 1.f);
		}
	}
	if (D->IsPractice())
	{
		Canvas.Label(D->Text(TEXT("practice"), TEXT("Practice")), FVector2D(12.f * S, 56.f * S), 11.f, FLinearColor(FColor(255, 110, 130)), 0.f);
	}
	else if (D->GetModeIndex() > 0)
	{
		Canvas.Label(D->CurrentMode().Name,FVector2D(12.f * S, 56.f * S), 11.f, FLinearColor(FColor(255, 110, 130)), 0.f);
	}
	if (D->IsGodMode() && !FParse::Param(FCommandLine::Get(), TEXT("KPCleanHud"))) // (T8: -KPCleanHud for the itch screenshots)
	{
		Canvas.Label(TEXT("god mode"),FVector2D(12.f * S, 76.f * S), 10.f, Gold, 0.f);
	}



	// Card pick between waves: name, the boon in green, the loose thread in rose
	if (D->GetMode() == EKPMode::Pick)
	{
		Canvas.Layer++;
		Canvas.Box(FVector2D::ZeroVector, Local, FLinearColor(FColor(30, 15, 40)).CopyWithNewOpacity(0.5f));
		Canvas.Layer++;
		const TArray<int32>& Offer = D->GetOffer();
		const bool bCursePick = Offer.Num() > 0 && D->GetCards().IsValidIndex(Offer[0]) && D->GetCards()[Offer[0]].Curse;
		if (Offer.Num() > 1)
		{
			Canvas.Label(bCursePick ? D->Text(TEXT("pickCurse"), TEXT("Pick tonight's curse")) : D->Text(TEXT("pickCard"), TEXT("Pick a card")),
				FVector2D(W * 0.5f, H * 0.2f), 22.f, FLinearColor::White);
		}
		for (int32 Slot = 0; Slot < Offer.Num(); ++Slot)
		{
			const FKPCard& Card = D->GetCards()[Offer[Slot]];
			const FBox2D Rect01 = KPCardLayout::CardRect(Slot, Offer.Num());
			FVector2D Pos(Rect01.Min.X * W, Rect01.Min.Y * H);
			const FVector2D Size((Rect01.Max.X - Rect01.Min.X) * W, (Rect01.Max.Y - Rect01.Min.Y) * H);
			const bool bHover = D->GetHoveredCard() == Slot;
			if (bHover)
			{
				Pos.Y -= 8.f * S;
				Canvas.Card(Pos - FVector2D(4.f, 4.f) * S, Size + FVector2D(8.f, 8.f) * S, Gold);
			}
			Canvas.Layer++;
			Canvas.PaperCard(Pos, Size, CardPaper);
			Canvas.Layer++;
			const float CenterX = Pos.X + Size.X * 0.5f;
			const float Inner = Size.X - 24.f * S;
			// Fixed slots, so the loose thread sits at the same height on every card: the name (a card already
			// held comes back as its next level, "Extra inch II"), a rule, the boon (room for two lines), then
			// "loose thread" and the catch. The boon needs no label: green is the good part.
			const float NameTop = Pos.Y + 22.f * S;
			Canvas.bHeading = true; // T7b: card names in the rounded heading font
			Canvas.Wrapped(KPCardText::Title(Card.Name, D->GetCardLevel(Offer[Slot]) + 1), CenterX, NameTop, Inner, 23.f, Ink, true);
			Canvas.bHeading = false;
			const float RuleY = NameTop + 46.f * S;
			Canvas.Box(FVector2D(Pos.X + 20.f * S, RuleY), FVector2D(Size.X - 40.f * S, 2.f * S), Muted.CopyWithNewOpacity(0.4f));
			const float BoonTop = RuleY + 22.f * S;
			if (Card.Curse)
			{
				// T7c: a curse: only the red part (the loose thread), in the same slot as on a card
				const float CurseTop = BoonTop + 24.f * S;
				const float CurseUsed = Canvas.Wrapped(D->Text(TEXT("curseLabel"), TEXT("curse")), CenterX, CurseTop, Inner, 12.f, Muted, false);
				Canvas.Wrapped(D->CardText(Offer[Slot], false), CenterX, CurseTop + CurseUsed + 4.f * S, Inner, 20.f, ThreadColor, true);
				Canvas.Layer -= 2;
				continue;
			}
			const FString Boon = D->CardText(Offer[Slot], true);
			Canvas.Wrapped(Boon, CenterX, BoonTop, Inner, Boon.Len() > 34 ? 15.5f : 18.f, BoonColor, true); // long ones smaller, to stay in two lines
			const float ThreadTop = BoonTop + 2.f * 24.f * S + 24.f * S;
			const float Used = Canvas.Wrapped(TEXT("loose thread"), CenterX, ThreadTop, Inner, 12.f, Muted, false);
			Canvas.Wrapped(D->CardText(Offer[Slot], false), CenterX, ThreadTop + Used + 4.f * S, Inner, 18.f, ThreadColor, true);
			Canvas.Layer -= 2;
		}
		Canvas.Layer += 3;
		Canvas.Label(bCursePick ? D->Text(TEXT("curseLasts"), TEXT("It lasts this night")) : D->Text(TEXT("cardsLast"), TEXT("Cards reset every night")),
			FVector2D(W * 0.5f, H * 0.78f), 12.f, FLinearColor::White);
	}

	// The tutorial (T6): one short prompt at the bottom of the screen. T6b: a fixed pill and a fixed font size
	// (no pulse); prompts only fade in and out
	const FString Tutorial = D->GetTutorialText();
	const float TutorialAlpha = D->GetTutorialAlpha();
	if (!Tutorial.IsEmpty() && TutorialAlpha > 0.f)
	{
		Canvas.Layer++;
		Canvas.Pill(FVector2D(W * 0.5f - 330.f * S, H - 58.f * S), FVector2D(660.f * S, 40.f * S), WithAlpha(FLinearColor(Ink.R, Ink.G, Ink.B, 0.75f), TutorialAlpha));
		Canvas.Layer++;
		Canvas.Plain(Tutorial, FVector2D(W * 0.5f, H - 38.f * S), 17.f, WithAlpha(Gold, TutorialAlpha), 0.5f, true);
	}

	// T6c: the 3-2-1 before a night
	if (D->GetCountdown() > 0.f && !D->IsGamePaused())
	{
		const float Left = D->GetCountdown();
		const int32 Number = FMath::CeilToInt(Left);
		const float Frac = Left - FMath::FloorToFloat(Left);
		const float Pop = 1.f + 0.35f * FMath::Clamp((Frac - 0.75f) / 0.25f, 0.f, 1.f);
		Canvas.Layer++;
		Canvas.Label(FString::Printf(TEXT("%d"), Number), FVector2D(W * 0.5f, H * 0.45f), 90.f * Pop, Gold, 0.5f, FMath::Clamp(Frac * 3.f + 0.2f, 0.f, 1.f));
	}

	// Pause (T6): over everything, the mat blurred behind it
	if (D->IsGamePaused())
	{
		PaintPause(Canvas, D, W, H);
	}

	return Canvas.Layer;
}

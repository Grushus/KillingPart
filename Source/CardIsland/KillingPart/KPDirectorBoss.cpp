// Killing Part: Sunday's boss, the first one (T6; the real fight since T6c). The first plushie she ever made,
// huge and faded. It ignores slow motion and walks at her; a touch costs a heart and knocks it back. Every line
// through it is one hit (bossHits of them); every chunkEvery hits a chunk comes off the side the line passed
// (AKPPlushie::ChopChunk) and gets up as a small teddy; the last hit splits it in half for good, with a big
// moment. Every hit also brings a plushie (or a pincushion) from the edges, and a flock of chicks comes in every
// chickEvery seconds, so she's never alone with it. Numbers: its type in KillingPart.json.

#include "KillingPart/KPDirector.h"
#include "KillingPart/KPPlushie.h"

void AKPDirector::SpawnBoss(const FString& TypeName, int32 Count, int32 HitsOverride)
{
	const int32 Index = FindType(TypeName);
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogKP, Warning, TEXT("No boss type called %s"), *TypeName);
		return;
	}
	const FKPPlushieType& Type = Types[Index];
	Bosses.Reset();
	BossPlushie.Reset();
	Count = FMath::Max(1, Count);
	for (int32 N = 0; N < Count; ++N)
	{
		const float BossRadius = FMath::FRandRange(Type.RadiusMin, Type.RadiusMax);
		// From the far edge (the top of the screen), half on the mat already; several spread across it
		const float Y = Count > 1 ? FMath::Lerp(-Tuning.ArenaHalfY * 0.6f, Tuning.ArenaHalfY * 0.6f, N / float(Count - 1)) : FMath::FRandRange(-300.f, 300.f);
		const FVector Pos(Tuning.ArenaHalfX + BossRadius * 0.5f + N * 60.f, Y, 0.f);
		const FLinearColor Color = KPVisuals::Hex(Type.Colors.Num() > 0 ? *Type.Colors[0] : TEXT("#D8C7AE"));
		if (AKPPlushie* Boss = SpawnPlushieAt(Index, Pos, BossRadius, Color, Type.BossSpeed * SpeedMul))
		{
			Boss->BossHitsLeft = FMath::Max(1, HitsOverride > 0 ? HitsOverride : Type.BossHits);
			Boss->BossHitsTaken = 0;
			Boss->bBossEnraged = false;
			Bosses.Add(Boss);
			if (!BossPlushie.IsValid())
			{
				BossPlushie = Boss;
			}
		}
	}
	PendingSpawns = 0;
	PendingChicks = 0;
	ChickTimer = Type.ChickEvery * 0.5f;
	bBossDown = false;
	EscortTimer = CurrentWave().EscortEvery * 0.6f;
	FString Name;
	FString Line;
	if (!Type.Intro.Split(TEXT(": "), &Name, &Line))
	{
		Name = Type.Name;
	}
	AddScreenText(FVector2D(0.5, 0.2), Count > 1 ? FString::Printf(TEXT("%s x%d"), *Name, Count) : Name, 2.6f, 32.f, KPVisuals::Hex(TEXT("#FFE14D")));
	UE_LOG(LogKP, Log, TEXT("Boss: %s x%d, %d hits each"), *Type.Name, Count, HitsOverride > 0 ? HitsOverride : Type.BossHits);
}

bool AKPDirector::AnyBossAlive() const
{
	for (const TWeakObjectPtr<AKPPlushie>& Each : Bosses)
	{
		if (Each.IsValid() && !Each->bDead)
		{
			return true;
		}
	}
	return false;
}

void AKPDirector::UpdateBoss(float GameDt, float RealDt)
{
	AKPPlushie* Boss = nullptr;
	for (const TWeakObjectPtr<AKPPlushie>& Each : Bosses)
	{
		if (Each.IsValid() && !Each->bDead)
		{
			Boss = Each.Get();
			break;
		}
	}
	if (!Boss || bBreakDone || Mode == EKPMode::Intro)
	{
		return;
	}
	BossPlushie = Boss; // (the HUD's)
	const FKPPlushieType* Type = Types.IsValidIndex(Boss->TypeIndex) ? &Types[Boss->TypeIndex] : nullptr;

	// An escort from the night's mix (0 = none)
	if (CurrentWave().EscortEvery > 0.f)
	{
		EscortTimer -= GameDt;
		if (EscortTimer <= 0.f)
		{
			EscortTimer = FMath::Max(0.3f, CurrentWave().EscortEvery);
			++PendingSpawns;
		}
	}

	// T6c: a flock of chicks every chickEvery seconds, from just outside the mat (they score nothing)
	if (Type && Type->ChickCount > 0)
	{
		ChickTimer -= GameDt;
		if (ChickTimer <= 0.f)
		{
			ChickTimer = FMath::Max(0.5f, Type->ChickEvery);
			PendingChicks = FMath::Min(PendingChicks + Type->ChickCount, Type->ChickCount * 2);
		}
	}

	// What the hits and the timers owe, as long as the mat isn't full (the alive cap)
	const int32 Golden = FindType(TEXT("golden"));
	const int32 Chick = FindType(TEXT("chick"));
	int32 Alive = AliveCount();
	while (PendingSpawns > 0 && Alive < Tuning.AliveCap)
	{
		--PendingSpawns;
		if (Type && FMath::FRand() < Type->HitPinShare)
		{
			SpawnPin();
			continue;
		}
		int32 Pick = PickType();
		for (int32 Try = 0; Try < 6 && Pick == Golden; ++Try)
		{
			Pick = PickType(); // (any type but the golden one)
		}
		if (Pick == Golden)
		{
			Pick = FindType(TEXT("teddy"));
		}
		Alive += SpawnPlushie(Pick) ? 1 : 0;
	}
	while (PendingChicks > 0 && Alive < Tuning.AliveCap && Chick != INDEX_NONE)
	{
		--PendingChicks;
		Alive += SpawnPlushie(Chick) ? 1 : 0;
	}

	// T6b: every chunk cut off gets up after a moment as a small teddy and rushes at her
	const int32 ChildType = Type ? FindType(Type->ChunkInto) : INDEX_NONE;
	const float LimitX = Tuning.ArenaHalfX - Tuning.EdgeMargin;
	const float LimitY = Tuning.ArenaHalfY - Tuning.EdgeMargin;
	for (int32 Index = BossChunks.Num() - 1; Index >= 0; --Index)
	{
		FBossChunk& Chunk = BossChunks[Index];
		Chunk.Timer -= GameDt;
		if (Chunk.Timer > 0.f)
		{
			continue;
		}
		FVector At = Chunk.At;
		AKPPlushie* ChunkBoss = Chunk.Owner.Get();
		if (IsValid(ChunkBoss))
		{
			ChunkBoss->RaiseChunk(Chunk.Index, At);
		}
		BossChunks.RemoveAt(Index);
		if (ChildType == INDEX_NONE)
		{
			continue;
		}
		const FKPPlushieType& Child = Types[ChildType];
		const FVector Pos(FMath::Clamp(At.X, -LimitX, LimitX), FMath::Clamp(At.Y, -LimitY, LimitY), 0.f);
		if (AKPPlushie* Cub = SpawnPlushieAt(ChildType, Pos, FMath::FRandRange(Child.RadiusMin, Child.RadiusMax), Boss->Color, Type->ChunkSpeed * SpeedMul))
		{
			Cub->PopIn = 0.3f;
			Cub->UpdatePose(FVector(-1.f, 0.f, 0.f), 0.f, false, 0.f, 0.01f);
			Burst(Cub->GetBodyCenter(), 10, FVector::ZeroVector, false);
			AddWorldText(Pos + FVector(0.f, 0.f, 120.f), TEXT("Up again!"), FString(), 0.9f, 13.f, FLinearColor::White);
		}
	}

	// T6b: the enrage tell: it shakes and its button eye glints
	for (const TWeakObjectPtr<AKPPlushie>& Each : Bosses)
	{
		AKPPlushie* Teller = Each.Get();
		if (!IsValid(Teller) || Teller->bDead || Teller->BossTell <= 0.f)
		{
			continue;
		}
		Teller->BossTell = FMath::Max(0.f, Teller->BossTell - RealDt);
		const float Amount = Teller->Radius * 0.06f;
		Teller->PoseJitter = Teller->BossTell > 0.f ? FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(0.f, 0.6f)) * Amount : FVector::ZeroVector;
		Teller->SetEyeGlint(Teller->BossTell > 0.f ? 1.f + 7.f * FMath::Abs(FMath::Sin(RealClock * 14.f)) : 1.f);
	}
}

int32 AKPDirector::GetBossCutsLeft() const
{
	// The hits left on every boss still standing
	int32 Left = 0;
	bool bAny = false;
	for (const TWeakObjectPtr<AKPPlushie>& Each : Bosses)
	{
		if (Each.IsValid() && !Each->bDead)
		{
			bAny = true;
			Left += FMath::Max(0, Each->BossHitsLeft);
		}
	}
	return bAny && !bAttract ? Left : -1;
}

FString AKPDirector::GetBossName() const
{
	const AKPPlushie* Boss = BossPlushie.Get();
	if (!IsValid(Boss) || !Types.IsValidIndex(Boss->TypeIndex))
	{
		return FString();
	}
	FString Name;
	FString Line;
	return Types[Boss->TypeIndex].Intro.Split(TEXT(": "), &Name, &Line) ? Name : Types[Boss->TypeIndex].Name;
}

int32 AKPDirector::CutBoss(AKPPlushie* Boss)
{
	const FKPPlushieType* Type = Types.IsValidIndex(Boss->TypeIndex) ? &Types[Boss->TypeIndex] : nullptr;
	++Boss->BossHitsTaken;
	--Boss->BossHitsLeft;
	PendingSpawns += Type ? Type->HitSpawns : 1; // every hit brings one more from the edges
	if (Boss->BossHitsLeft <= 0)
	{
		// The last hit: in half for good, through the middle (the big finish when it's the last boss standing)
		Boss->CutOffset = 0.f;
		Boss->bDead = true;
		SliceQueue.Add(Boss);
		if (AnyBossAlive())
		{
			Burst(Boss->GetBodyCenter(), 40, FVector::ZeroVector, !KPSettings::IsTheatrical());
			Shake = FMath::Max(Shake, 22.f);
		}
		else
		{
			StartFinale(Boss);
		}
		UE_LOG(LogKP, Log, TEXT("Boss: split (%d hits)"), Boss->BossHitsTaken);
		return Type ? Type->SplitScore : 3000;
	}

	int32 Points = Type ? Type->HitScore : 100;
	const bool bBlood = !KPSettings::IsTheatrical();
	const int32 Every = Type ? FMath::Max(1, Type->ChunkEvery) : 10;
	if (Boss->BossHitsTaken % Every == 0)
	{
		// A chunk off the side the line passed (straight through the middle: either side)
		const float Side = FMath::Abs(Boss->CutOffset) > 1.f ? FMath::Sign(Boss->CutOffset) : (FMath::RandBool() ? 1.f : -1.f);
		const FVector Outward = KPMath::CutNormal(Boss->CutYaw) * Side;
		FVector ChunkAt = Boss->GetBodyCenter();
		if (Boss->ChopChunk(Outward, Type ? Type->ChunkOffset : 0.6f, CapBase, Tuning, ChunkAt))
		{
			Burst(ChunkAt, bBlood ? 34 : 20, Outward, bBlood);
			Spray(ChunkAt, Outward, bBlood);
			if (bBlood)
			{
				StainSplat(FVector(ChunkAt.X, ChunkAt.Y, 0.f), Boss->Radius * 0.45f, Outward);
			}
			if (Type && !Type->ChunkInto.IsEmpty())
			{
				// T6b: it gets up in a moment (UpdateBoss); if the chunk is gone by then, where it flew to roughly
				FBossChunk Pending;
				Pending.Owner = Boss;
				Pending.Index = Boss->GetNumChunks() - 1;
				Pending.At = ChunkAt + Outward * Boss->Radius * 0.8f;
				Pending.Timer = Type->ChunkDelay;
				BossChunks.Add(Pending);
			}
		}
		else
		{
			UE_LOG(LogKP, Warning, TEXT("Boss: no chunk came off"));
		}
		Boss->SetSizeScale(Boss->GetSizeScale() * (1.f - (Type ? Type->ShrinkPerCut : 0.06f)));
		Shake = FMath::Max(Shake, 14.f);
		Points += Type ? Type->ChunkScore : 500;
		UE_LOG(LogKP, Log, TEXT("Boss: chunk off, %d hits left"), Boss->BossHitsLeft);
	}
	else
	{
		// A plain hit: a spurt from the line, a little shake
		const FVector Normal = KPMath::CutNormal(Boss->CutYaw);
		Burst(Boss->GetBodyCenter() + Normal * Boss->CutOffset, bBlood ? 12 : 8, Normal, bBlood);
		Shake = FMath::Max(Shake, 6.f);
	}
	Boss->Unmark();
	const int32 EnrageAt = Type ? FMath::Min(Type->EnrageAt, FMath::RoundToInt((Boss->BossHitsTaken + Boss->BossHitsLeft) * 0.4f)) : 0; // (at most 40% of its hits: Sunday 20 of 50)
	if (Type && !Boss->bBossEnraged && Boss->BossHitsLeft <= EnrageAt)
	{
		// T6b: enraged: faster, and a short tell (it shakes, its button eye glints)
		Boss->bBossEnraged = true;
		Boss->Speed *= Type->EnrageSpeed;
		Boss->BossTell = Type->EnrageTell;
		PlaySound(TEXT("enrage")); // T7b
		AddWorldText(Boss->GetBodyCenter() + FVector(0.f, 0.f, Boss->Radius + 40.f), Text(TEXT("bossEnraged"), TEXT("It's angry!")), FString(), 1.4f, 20.f, KPVisuals::Hex(TEXT("#FF4F8F")));
		UE_LOG(LogKP, Log, TEXT("Boss: enraged at %d hits left, speed %.0f"), Boss->BossHitsLeft, Boss->Speed);
	}
	return Points;
}

void AKPDirector::StartFinale(AKPPlushie* Boss)
{
	// Held slow motion, and everything still on the mat falls apart with it (no score: the moment is the boss's)
	FinaleTimer = 2.4f;
	bBossDown = true;
	CineBoss = Boss;
	StartCine(2); // T7d: a camera cut on the split (until the slow motion ends)
	PlaySound(TEXT("killingpart")); // T7: the final split
	BossChunks.Reset(); // (chunks still lying there stay down)
	PendingSpawns = 0;
	PendingChicks = 0;
	Boss->PoseJitter = FVector::ZeroVector;
	Boss->BossTell = 0.f;
	Invulnerable = FMath::Max(Invulnerable, 5.f);
	SpawnList.Reset();
	Queue = 0;
	PathPinsLeft = 0;
	for (AKPPlushie* Plushie : Plushies)
	{
		if (IsValid(Plushie) && Plushie != Boss && !Plushie->bDead && !Plushie->bMarked)
		{
			Plushie->SetMarked(FMath::FRand() * 360.f, 0.f);
			Plushie->bDead = true;
			SliceQueue.Add(Plushie);
		}
	}
	FadeOutPins();
	const bool bBlood = !KPSettings::IsTheatrical();
	const FVector Center = Boss->GetBodyCenter();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector Offset(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-0.2f, 0.6f));
		Burst(Center + Offset * Boss->Radius * 0.6f, 40, FVector::ZeroVector, bBlood);
	}
	const FVector Normal = KPMath::CutNormal(Boss->CutYaw);
	Spray(Center, Normal, bBlood);
	Spray(Center + FVector(0.f, 0.f, Boss->Radius * 0.3f), Normal.RotateAngleAxis(30.f, FVector::UpVector), bBlood);
	Flash = 0.6f;
	FlashColor = FLinearColor(FColor(255, 236, 170));
	Shake = Tuning.FinaleShake;
	HitStopTimer = Tuning.HitStopFinale; // T7b
	AddScreenText(FVector2D(0.5, 0.36), Text(TEXT("killingPart"), TEXT("Killing part")), 3.4f, 56.f, KPVisuals::Hex(TEXT("#FFE14D")));
}

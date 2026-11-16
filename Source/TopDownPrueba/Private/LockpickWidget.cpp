#include "LockpickWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

void ULockpickWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetKeyboardFocus();
	StartLockpick(Difficulty, StartingPicks);
}

void ULockpickWidget::StartLockpick(ELockDifficulty NewDifficulty, int32 NewPicks)
{
	Difficulty = NewDifficulty;
	PicksRemaining = FMath::Max(1, NewPicks);
	SweetSpot = FMath::FRandRange(-MaxSweetSpotAngle, MaxSweetSpotAngle);
	LockAngle = 0.f;
	PickStress = 0.f;
	bTurning = false;
	bMouseHeld = false;
	bFinished = false;

	UE_LOG(LogTemp, Warning, TEXT(">>> Lockpick: SweetSpot=%f Tol=%f"), SweetSpot, GetTolerance());
	RefreshPicksText();
}

float ULockpickWidget::GetTolerance() const
{
	switch (Difficulty)
	{
	case ELockDifficulty::Novice:     return 14.f;
	case ELockDifficulty::Apprentice: return 10.f;
	case ELockDifficulty::Adept:      return 7.f;
	case ELockDifficulty::Expert:     return 4.5f;
	case ELockDifficulty::Master:     return 2.5f;
	}
	return 10.f;
}

void ULockpickWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bFinished)
	{
		return;
	}

	// Teclado por polling: no depende de tener el foco de Slate.
	bool bKeyHeld = false;
	if (const APlayerController* PC = GetOwningPlayer())
	{
		bKeyHeld = PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::D)
			|| PC->IsInputKeyDown(EKeys::SpaceBar)
			|| PC->IsInputKeyDown(EKeys::Left) || PC->IsInputKeyDown(EKeys::Right);
	}
	bTurning = bMouseHeld || bKeyHeld;

	// La ganzúa se congela mientras la cerradura gira.
	if (!bTurning)
	{
		PickAngle = FMath::FInterpTo(PickAngle, TargetPickAngle, InDeltaTime, PickFollowSpeed);
	}

	const float Error = FMath::Max(0.f, FMath::Abs(PickAngle - SweetSpot) - GetTolerance());
	const float Allowed = FMath::Lerp(90.f, MinJamAngle, FMath::Clamp(Error / ForgivenessRange, 0.f, 1.f));

	float Shake = 0.f;

	if (bTurning)
	{
		LockAngle = FMath::Min(LockAngle + TurnSpeed * InDeltaTime, Allowed);

		if (LockAngle >= 90.f - KINDA_SMALL_NUMBER)
		{
			Finish(true);
			return;
		}

		if (LockAngle >= Allowed - KINDA_SMALL_NUMBER)
		{
			// Trabada: vibra y acumula tensión.
			PickStress += InDeltaTime;
			Shake = FMath::Sin(GetWorld()->GetTimeSeconds() * 60.f) * ShakeDegrees;

			if (PickStress >= PickBreakTime)
			{
				BreakPick();
				return;
			}
		}
		else
		{
			PickStress = 0.f;
		}
	}
	else
	{
		LockAngle = FMath::Max(0.f, LockAngle - ReturnSpeed * InDeltaTime);
		PickStress = 0.f;
	}

	Image_Lock->SetRenderTransformAngle(LockAngle);
	Image_Pick->SetRenderTransformAngle(PickAngle + Shake);
}

void ULockpickWidget::BreakPick()
{
	PicksRemaining--;
	LockAngle = 0.f;
	PickStress = 0.f;
	bTurning = false;
	bMouseHeld = false;
	RefreshPicksText();
	OnPickBroken.Broadcast();

	if (PicksRemaining <= 0)
	{
		Finish(false);
	}
}

void ULockpickWidget::Finish(bool bSuccess)
{
	bFinished = true;
	bTurning = false;
	bMouseHeld = false;

	if (bSuccess)
	{
		OnLockpickSuccess.Broadcast();
	}
	else
	{
		OnLockpickFailed.Broadcast();
	}
}

void ULockpickWidget::RefreshPicksText()
{
	if (Text_Picks)
	{
		Text_Picks->SetText(FText::AsNumber(PicksRemaining));
	}
}

FReply ULockpickWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FVector2D Local = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	const float Alpha = FMath::Clamp(Local.X / FMath::Max(1.f, InGeometry.GetLocalSize().X), 0.f, 1.f);
	TargetPickAngle = FMath::Lerp(-90.f, 90.f, Alpha);
	return FReply::Handled();
}

FReply ULockpickWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SetKeyboardFocus();
		bMouseHeld = true;
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply ULockpickWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bMouseHeld = false;
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
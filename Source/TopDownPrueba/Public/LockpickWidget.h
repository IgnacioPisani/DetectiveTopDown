#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LockpickWidget.generated.h"

class UImage;
class UTextBlock;

UENUM(BlueprintType)
enum class ELockDifficulty : uint8
{
	Novice,
	Apprentice,
	Adept,
	Expert,
	Master
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLockpickEvent);

UCLASS()
class ULockpickWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Lockpick")
	FOnLockpickEvent OnLockpickSuccess;

	UPROPERTY(BlueprintAssignable, Category = "Lockpick")
	FOnLockpickEvent OnLockpickFailed;

	UPROPERTY(BlueprintAssignable, Category = "Lockpick")
	FOnLockpickEvent OnPickBroken;

	// Reinicia la cerradura con nuevo punto dulce.
	UFUNCTION(BlueprintCallable, Category = "Lockpick")
	void StartLockpick(ELockDifficulty NewDifficulty, int32 NewPicks);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// --- Configuración (Class Defaults del WBP) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockpick")
	ELockDifficulty Difficulty = ELockDifficulty::Novice;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockpick", meta = (ClampMin = "1"))
	int32 StartingPicks = 5;

	// Segundos trabada antes de que se rompa la ganzúa.
	UPROPERTY(EditAnywhere, Category = "Lockpick")
	float PickBreakTime = 0.9f;

	// Grados por segundo al girar la cerradura.
	UPROPERTY(EditAnywhere, Category = "Lockpick")
	float TurnSpeed = 90.f;

	UPROPERTY(EditAnywhere, Category = "Lockpick")
	float ReturnSpeed = 180.f;

	// Qué tan rápido sigue la ganzúa al mouse.
	UPROPERTY(EditAnywhere, Category = "Lockpick")
	float PickFollowSpeed = 20.f;

	// Fuera de la tolerancia, cuántos grados hasta que la cerradura casi no gire.
	UPROPERTY(EditAnywhere, Category = "Lockpick")
	float ForgivenessRange = 40.f;

	// Giro mínimo permitido aun lejos del punto dulce.
	UPROPERTY(EditAnywhere, Category = "Lockpick")
	float MinJamAngle = 8.f;

	UPROPERTY(EditAnywhere, Category = "Lockpick")
	float ShakeDegrees = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Lockpick")
	float MaxSweetSpotAngle = 75.f;

	// --- Widgets (nombres exactos, Is Variable) ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_Lock;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_Pick;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Picks;

private:
	float GetTolerance() const;
	void BreakPick();
	void Finish(bool bSuccess);
	void RefreshPicksText();

	float SweetSpot = 0.f;
	float PickAngle = 0.f;
	float TargetPickAngle = 0.f;
	float LockAngle = 0.f;
	float PickStress = 0.f;
	int32 PicksRemaining = 0;
	bool bTurning = false;
	bool bFinished = false;
};
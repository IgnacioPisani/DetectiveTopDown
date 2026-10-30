#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TelefonoWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTelefonoCodeComplete, const FString&, Code);

UCLASS()
class UTelefonoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Se dispara cuando se completan los caracteres máximos.
	UPROPERTY(BlueprintAssignable, Category = "Telefono")
	FOnTelefonoCodeComplete OnCodeComplete;

	UFUNCTION(BlueprintCallable, Category = "Telefono")
	void ClearDisplay();

	UFUNCTION(BlueprintPure, Category = "Telefono")
	FString GetCurrentCode() const { return CurrentCode; }

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telefono", meta = (ClampMin = "1"))
	int32 MaxChars = 4;

	// Los nombres deben coincidir EXACTO con los del widget.
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> TextBlock_Display;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_1;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_2;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_3;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_4;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_5;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_6;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_7;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_8;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_9;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_10; // *
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_11; // 0
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Button_12; // #

private:
	void AppendChar(TCHAR C);
	void RefreshDisplay();

	// Los delegates dinámicos de UButton no aceptan parámetros ni lambdas, por eso un handler por botón.
	UFUNCTION() void OnBtn1();
	UFUNCTION() void OnBtn2();
	UFUNCTION() void OnBtn3();
	UFUNCTION() void OnBtn4();
	UFUNCTION() void OnBtn5();
	UFUNCTION() void OnBtn6();
	UFUNCTION() void OnBtn7();
	UFUNCTION() void OnBtn8();
	UFUNCTION() void OnBtn9();
	UFUNCTION() void OnBtnStar();
	UFUNCTION() void OnBtn0();
	UFUNCTION() void OnBtnHash();

	FString CurrentCode;
};
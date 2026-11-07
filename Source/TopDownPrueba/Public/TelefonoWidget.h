#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TelefonoWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTelefonoCodeComplete, const FString&, Code);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTelefonoCallResult);

UCLASS()
class UTelefonoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Se dispara cuando se completan los caracteres máximos.
	UPROPERTY(BlueprintAssignable, Category = "Telefono")
	FOnTelefonoCodeComplete OnCodeComplete;

	// Se dispara al apretar "llamar" con el código correcto.
	UPROPERTY(BlueprintAssignable, Category = "Telefono")
	FOnTelefonoCallResult OnCallSuccess;

	// Se dispara al apretar "llamar" con un código incorrecto.
	UPROPERTY(BlueprintAssignable, Category = "Telefono")
	FOnTelefonoCallResult OnCallFailed;

	UFUNCTION(BlueprintCallable, Category = "Telefono")
	void ClearDisplay();

	UFUNCTION(BlueprintPure, Category = "Telefono")
	FString GetCurrentCode() const { return CurrentCode; }

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telefono", meta = (ClampMin = "1"))
	int32 MaxChars = 4;

	// Código que desbloquea el teléfono.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telefono")
	FString CorrectCode = TEXT("1905");

	// Imagen que reemplaza a Image_0 cuando el código es correcto.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telefono")
	TObjectPtr<UTexture2D> SuccessImage;


	// Los nombres deben coincidir EXACTO con los del widget.
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Image_Light;
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

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_borrar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_llamar;

private:
	void AppendChar(TCHAR C);
	void RefreshDisplay();

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

	UFUNCTION() void OnBorrar();
	UFUNCTION() void OnLlamar();

	FString CurrentCode;
	bool bUnlocked = false;
};
#include "TelefonoWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UTelefonoWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_1->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtn1);
	Button_2->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtn2);
	Button_3->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtn3);
	Button_4->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtn4);
	Button_5->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtn5);
	Button_6->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtn6);
	Button_7->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtn7);
	Button_8->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtn8);
	Button_9->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtn9);
	Button_10->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtnStar);
	Button_11->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtn0);
	Button_12->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBtnHash);

	Btn_borrar->OnClicked.AddDynamic(this, &UTelefonoWidget::OnBorrar);
	Btn_llamar->OnClicked.AddDynamic(this, &UTelefonoWidget::OnLlamar);

	RefreshDisplay();
}

void UTelefonoWidget::AppendChar(TCHAR C)
{
	if (bUnlocked || CurrentCode.Len() >= MaxChars)
	{
		return;
	}

	CurrentCode.AppendChar(C);
	RefreshDisplay();

	if (CurrentCode.Len() == MaxChars)
	{
		OnCodeComplete.Broadcast(CurrentCode);
	}
}

void UTelefonoWidget::ClearDisplay()
{
	CurrentCode.Empty();
	RefreshDisplay();
}

void UTelefonoWidget::RefreshDisplay()
{
	if (TextBlock_Display)
	{
		TextBlock_Display->SetText(FText::FromString(CurrentCode));
	}
}

void UTelefonoWidget::OnBorrar()
{
	if (bUnlocked || CurrentCode.IsEmpty())
	{
		return;
	}

	CurrentCode.LeftChopInline(1); // saca el último carácter
	RefreshDisplay();
}

void UTelefonoWidget::OnLlamar()
{
	if (bUnlocked)
	{
		return;
	}

	if (CurrentCode == CorrectCode)
	{
		bUnlocked = true;

		if (Image_Light && SuccessImage)
		{
			Image_Light->SetBrushFromTexture(SuccessImage);
		}
		if (TextBlock_Display)
		{
			TextBlock_Display->SetText(FText::FromString(TEXT("CALLING...")));
		}
		OnCallSuccess.Broadcast();
	}
	else
	{
		ClearDisplay();
		OnCallFailed.Broadcast();
	}
}

void UTelefonoWidget::OnBtn1()    { AppendChar(TEXT('1')); }
void UTelefonoWidget::OnBtn2()    { AppendChar(TEXT('2')); }
void UTelefonoWidget::OnBtn3()    { AppendChar(TEXT('3')); }
void UTelefonoWidget::OnBtn4()    { AppendChar(TEXT('4')); }
void UTelefonoWidget::OnBtn5()    { AppendChar(TEXT('5')); }
void UTelefonoWidget::OnBtn6()    { AppendChar(TEXT('6')); }
void UTelefonoWidget::OnBtn7()    { AppendChar(TEXT('7')); }
void UTelefonoWidget::OnBtn8()    { AppendChar(TEXT('8')); }
void UTelefonoWidget::OnBtn9()    { AppendChar(TEXT('9')); }
void UTelefonoWidget::OnBtnStar() { AppendChar(TEXT('*')); }
void UTelefonoWidget::OnBtn0()    { AppendChar(TEXT('0')); }
void UTelefonoWidget::OnBtnHash() { AppendChar(TEXT('#')); }
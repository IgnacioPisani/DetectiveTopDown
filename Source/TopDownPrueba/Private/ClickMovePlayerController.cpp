#include "ClickMovePlayerController.h"

#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "PickableComponent.h"
#include "SubSceneFirstPersonPawn.h"

AClickMovePlayerController::AClickMovePlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AClickMovePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AClickMovePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ClickAction)
		{
			EIC->BindAction(ClickAction, ETriggerEvent::Started, this, &AClickMovePlayerController::OnClickPressed);
		}
		if (ClickHoldAction)
		{
			EIC->BindAction(ClickHoldAction, ETriggerEvent::Started, this, &AClickMovePlayerController::OnClickHoldStarted);
			EIC->BindAction(ClickHoldAction, ETriggerEvent::Completed, this, &AClickMovePlayerController::OnClickHoldReleased);
		}
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AClickMovePlayerController::OnLook);
		}
		if (CancelAction)
		{
			EIC->BindAction(CancelAction, ETriggerEvent::Started, this, &AClickMovePlayerController::OnCancel);
		}
	}
}

// ------------------------------------------------------------------------
// Input handlers
// ------------------------------------------------------------------------

void AClickMovePlayerController::OnClickPressed()
{
	// En modo Examine el click no hace trace normal: arrastrar (ClickHoldAction)
	// es lo que rota el objeto. Un click "suelto" acá no hace nada.
	if (GetCurrentMode() == EInteractionMode::Examine)
	{
		return;
	}

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit) || !Hit.GetActor())
	{
		return;
	}

	if (UPickableComponent* Pickable = Hit.GetActor()->FindComponentByClass<UPickableComponent>())
	{
		if (Pickable->bIsPickable)
		{
			HandlePickableInteraction(Hit.GetActor(), Pickable);
			return;
		}
	}

	// No es pickeable: solo movemos al personaje en el nivel top-down (stack vacío).
	// Dentro de una sub-escena, clickear algo que no es pickeable no hace nada.
	if (!IsFocused())
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Hit.Location);
	}
}

void AClickMovePlayerController::OnClickHoldStarted()
{
	UE_LOG(LogTemp, Warning, TEXT("ClickHold Started, mode=%d"), (int32)GetCurrentMode());
	if (GetCurrentMode() == EInteractionMode::Examine)
	{
		bIsDraggingExamine = true;
	}
}

void AClickMovePlayerController::OnClickHoldReleased()
{
	bIsDraggingExamine = false;
}

void AClickMovePlayerController::OnLook(const FInputActionValue& Value)
{
	const FVector2D Delta = Value.Get<FVector2D>();
	UE_LOG(LogTemp, Warning, TEXT("OnLook delta=%s dragging=%d mode=%d"), *Delta.ToString(), bIsDraggingExamine, (int32)GetCurrentMode());
	const EInteractionMode Mode = GetCurrentMode();

	if (Mode == EInteractionMode::Examine && bIsDraggingExamine && FocusStack.Num() > 0)
	{
		AActor* Target = FocusStack.Last().ExamineTarget;
		if (!Target)
		{
			return;
		}

		const UPickableComponent* Pickable = Target->FindComponentByClass<UPickableComponent>();
		const float Speed = Pickable ? Pickable->ExamineRotationSpeed : 0.5f;

		FRotator NewRotation = Target->GetActorRotation();
		NewRotation.Yaw += Delta.X * Speed;
		NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch - Delta.Y * Speed, -60.f, 60.f);
		Target->SetActorRotation(NewRotation);
	}
	else if (Mode == EInteractionMode::EnterSubScene)
	{
		if (ASubSceneFirstPersonPawn* FPPawn = Cast<ASubSceneFirstPersonPawn>(GetPawn()))
		{
			FPPawn->AddLookInput(Delta);
		}
	}
}

void AClickMovePlayerController::OnCancel()
{
	PopFocus();
}

// ------------------------------------------------------------------------
// Interacción con pickeables
// ------------------------------------------------------------------------

void AClickMovePlayerController::HandlePickableInteraction(AActor* Target, UPickableComponent* Pickable)
{
	switch (Pickable->Mode)
	{
	case EInteractionMode::ShowUI:
		PushShowUI(Pickable->WidgetClass);
		break;

	case EInteractionMode::FirstPersonView:
		PushFirstPersonCamera(Pickable->FocusCamera);
		break;

	case EInteractionMode::EnterSubScene:
		PushSubScene(Pickable->SubScenePawnClass, Pickable->SubSceneEntryPoint);
		break;

	case EInteractionMode::Examine:
		PushExamine(Target);
		break;

	case EInteractionMode::SimpleEvent:
		Pickable->OnInteractEvent.Broadcast();
		break;

	default:
		break;
	}
}

// ------------------------------------------------------------------------
// Stack de foco
// ------------------------------------------------------------------------

void AClickMovePlayerController::PushShowUI(TSubclassOf<UUserWidget> WidgetClass)
{
	if (!WidgetClass)
	{
		return;
	}

	FFocusLayer Layer;
	Layer.Mode = EInteractionMode::ShowUI;
	Layer.PreviousPawn = GetPawn();
	Layer.Widget = CreateWidget<UUserWidget>(this, WidgetClass);

	if (Layer.Widget)
	{
		Layer.Widget->AddToViewport();

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(Layer.Widget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}

	FocusStack.Add(Layer);
}

void AClickMovePlayerController::PushFirstPersonCamera(ACameraActor* Camera)
{
	if (!Camera)
	{
		return;
	}

	FFocusLayer Layer;
	Layer.Mode = EInteractionMode::FirstPersonView;
	Layer.PreviousPawn = GetPawn();
	Layer.PreviousViewTarget = GetViewTarget();

	SetViewTargetWithBlend(Camera, 0.5f, EViewTargetBlendFunction::VTBlend_EaseInOut);

	FocusStack.Add(Layer);
}

void AClickMovePlayerController::PushSubScene(TSubclassOf<APawn> PawnClass, AActor* EntryPoint)
{
	OnSubSceneStateChanged.Broadcast(true);
	if (!PawnClass || !EntryPoint || !GetWorld())
	{
		return;
	}

	FFocusLayer Layer;
	Layer.Mode = EInteractionMode::EnterSubScene;
	Layer.PreviousPawn = GetPawn();
	Layer.PreviousViewTarget = GetViewTarget();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APawn* NewPawn = GetWorld()->SpawnActor<APawn>(
		PawnClass, EntryPoint->GetActorLocation(), EntryPoint->GetActorRotation(), SpawnParams);

	if (!NewPawn)
	{
		return;
	}

	Layer.SpawnedPawn = NewPawn;

	// Escondemos el pawn de top-down en vez de destruirlo: conserva posición y estado
	// para cuando volvamos con PopFocus().
	if (APawn* PawnToHide = Layer.PreviousPawn)
	{
		PawnToHide->SetActorHiddenInGame(true);
		PawnToHide->SetActorEnableCollision(false);
		PawnToHide->SetActorTickEnabled(false);
	}

	Possess(NewPawn);
	SetViewTargetWithBlend(NewPawn, 0.5f, EViewTargetBlendFunction::VTBlend_EaseInOut);

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = true; // seguimos necesitando el cursor para clickear objetos en la sub-escena

	FocusStack.Add(Layer);
}

void AClickMovePlayerController::PushExamine(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	FFocusLayer Layer;
	Layer.Mode = EInteractionMode::Examine;
	Layer.ExamineTarget = TargetActor;
	Layer.ExamineOriginalRotation = TargetActor->GetActorRotation();

	// Punto de extensión: acá es donde podrías hacer un SetViewTargetWithBlend
	// a una "cámara de mano" acercada al objeto, si querés ese efecto visual.

	FocusStack.Add(Layer);
}

void AClickMovePlayerController::PopFocus()
{
	if (FocusStack.Num() == 0)
	{
		return;
	}

	const FFocusLayer Layer = FocusStack.Pop();

	switch (Layer.Mode)
	{
	case EInteractionMode::ShowUI:
		if (Layer.Widget)
		{
			Layer.Widget->RemoveFromParent();
		}
		if (IsFocused())
		{
			SetInputMode(FInputModeUIOnly());
		}
		else
		{
			SetInputMode(FInputModeGameOnly());
		}
		break;
	case EInteractionMode::FirstPersonView:
		if (Layer.PreviousViewTarget)
		{
			SetViewTargetWithBlend(Layer.PreviousViewTarget, 0.5f, EViewTargetBlendFunction::VTBlend_EaseInOut);
		}
		break;

	case EInteractionMode::EnterSubScene:
		OnSubSceneStateChanged.Broadcast(false);
		if (Layer.PreviousPawn)
		{
			Layer.PreviousPawn->SetActorHiddenInGame(false);
			Layer.PreviousPawn->SetActorEnableCollision(true);
			Layer.PreviousPawn->SetActorTickEnabled(true);

			Possess(Layer.PreviousPawn);
			SetViewTargetWithBlend(Layer.PreviousPawn, 0.5f, EViewTargetBlendFunction::VTBlend_EaseInOut);
		}
		if (Layer.SpawnedPawn)
		{
			Layer.SpawnedPawn->Destroy();
		}
		bShowMouseCursor = true;
		SetInputMode(FInputModeGameOnly());
		break;

	case EInteractionMode::Examine:
		if (Layer.ExamineTarget)
		{
			Layer.ExamineTarget->SetActorRotation(Layer.ExamineOriginalRotation);
		}
		break;

	default:
		break;
	}
}

EInteractionMode AClickMovePlayerController::GetCurrentMode() const
{
	return FocusStack.Num() > 0 ? FocusStack.Last().Mode : EInteractionMode::None;
}
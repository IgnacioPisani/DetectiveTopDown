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
	
		if (MouseLookMappingContext)
		{
			Subsystem->AddMappingContext(MouseLookMappingContext, 0);
		}
	}
	SetInputMode(FInputModeGameOnly());

	ReleaseMouseCapture();   // <-- top-down empieza SIN captura forzada

}


void AClickMovePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ClickAction)
		{
			EIC->BindAction(ClickAction, ETriggerEvent::Started, this, &AClickMovePlayerController::OnInputStarted);
			EIC->BindAction(ClickAction, ETriggerEvent::Triggered, this, &AClickMovePlayerController::OnSetDestinationTriggered);
			EIC->BindAction(ClickAction, ETriggerEvent::Completed, this, &AClickMovePlayerController::OnSetDestinationReleased);
			EIC->BindAction(ClickAction, ETriggerEvent::Canceled, this, &AClickMovePlayerController::OnSetDestinationReleased);
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



void AClickMovePlayerController::OnInputStarted()
{
	// Si el click cae sobre un pickable, o estamos enfocados, lo absorbe
	// la interacción y no arrancamos ningún movimiento.
	if (IsFocused())
	{
		return;
	}

	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit) && Hit.GetActor())
	{
		if (UPickableComponent* Pickable = Hit.GetActor()->FindComponentByClass<UPickableComponent>())
		{
			if (Pickable->bIsPickable)
			{
				HandlePickableInteraction(Hit.GetActor(), Pickable);
				return;
			}
		}
	}

	StopMovement();
}

void AClickMovePlayerController::OnSetDestinationTriggered()
{
	if (IsFocused())
	{
		return;
	}

	FollowTime += GetWorld()->GetDeltaSeconds();

	FVector FloorLocation;
	if (!GetFloorLocationUnderCursor(FloorLocation))
	{
		return;
	}

	CachedDestination = FloorLocation;

	if (APawn* ControlledPawn = GetPawn())
	{
		const FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void AClickMovePlayerController::OnSetDestinationReleased()
{
	if (IsFocused())
	{
		return;
	}

	// Click corto: navega al punto por NavMesh. Click sostenido/arrastrado:
	// ya nos movimos frame a frame en OnSetDestinationTriggered, no hace falta más.
	if (FollowTime <= ShortPressThreshold)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
	}

	FollowTime = 0.f;
}

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

			// Si este mismo click nos metió en modo Examine, arrancamos el
			// arrastre ya mismo (el botón sigue presionado en este frame).
			if (GetCurrentMode() == EInteractionMode::Examine)
			{
				bIsDraggingExamine = true;
				UE_LOG(LogTemp, Warning, TEXT("Entered Examine + forced dragging=true"));

			}
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
	const EInteractionMode Mode = GetCurrentMode();

	if (Mode == EInteractionMode::Examine && IsInputKeyDown(EKeys::LeftMouseButton))
	{
		AActor* Target = FocusStack.Last().ExamineTarget;
		if (!Target)
		{
			return;
		}

		const UPickableComponent* Pickable = Target->FindComponentByClass<UPickableComponent>();
		const float Speed = Pickable ? Pickable->ExamineRotationSpeed : 0.5f;

		ASubSceneFirstPersonPawn* FPPawn = Cast<ASubSceneFirstPersonPawn>(GetPawn());
		if (!FPPawn)
		{
			return;
		}

		const FQuat CameraQuat = FPPawn->GetCameraRotation().Quaternion();
		const FVector CameraRight = CameraQuat.GetRightVector();
		const FVector CameraUp = CameraQuat.GetUpVector();

		const FQuat YawDelta = FQuat(CameraUp, FMath::DegreesToRadians(Delta.X * Speed));
		const FQuat PitchDelta = FQuat(CameraRight, FMath::DegreesToRadians(-Delta.Y * Speed));

		const FQuat CurrentQuat = Target->GetActorQuat();
		const FQuat NewQuat = YawDelta * PitchDelta * CurrentQuat;
		Target->SetActorRotation(NewQuat);
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

void AClickMovePlayerController::ApplyPersistentMouseCapture()
{
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently);
		Viewport->SetMouseLockMode(EMouseLockMode::LockAlways);

		UE_LOG(LogTemp, Warning, TEXT("MouseCapture set. Current CaptureMode=%d LockMode=%d"),
			(int32)Viewport->GetMouseCaptureMode(), (int32)Viewport->GetMouseLockMode());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyPersistentMouseCapture: no GameViewport found!"));
	}
}

void AClickMovePlayerController::ReleaseMouseCapture()
{
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->SetMouseCaptureMode(EMouseCaptureMode::CaptureDuringMouseDown);
		Viewport->SetMouseLockMode(EMouseLockMode::DoNotLock);
	}
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

bool AClickMovePlayerController::GetFloorLocationUnderCursor(FVector& OutLocation) const
{
	FVector WorldOrigin, WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	// Usamos la altura Z actual del pawn como referencia del plano del piso.
	// Sirve mientras el piso sea plano (sin escalones/rampas); si tu escena
	// tiene desniveles, este approach necesitaría ajustarse por zona.
	const APawn* ControlledPawn = GetPawn();
	const float FloorZ = ControlledPawn ? ControlledPawn->GetActorLocation().Z : 0.f;

	const FPlane FloorPlane(FVector(0.f, 0.f, FloorZ), FVector::UpVector);
	OutLocation = FMath::RayPlaneIntersection(WorldOrigin, WorldDirection, FloorPlane);
	return true;
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
	ApplyPersistentMouseCapture();   // <-- agregar esta línea acá

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
	Layer.ExamineOriginalLocation = TargetActor->GetActorLocation();
	Layer.ExamineOriginalRotation = TargetActor->GetActorRotation();

	// Lo movemos al centro de la pantalla, delante de la cámara actual.
	if (ASubSceneFirstPersonPawn* FPPawn = Cast<ASubSceneFirstPersonPawn>(GetPawn()))
	{
		const UPickableComponent* Pickable = TargetActor->FindComponentByClass<UPickableComponent>();
		const float Distance = Pickable ? Pickable->ExamineDistance : 150.f;

		const FVector CameraLocation = FPPawn->GetCameraLocation();
		const FVector CameraForward = FPPawn->GetCameraRotation().Vector();

		TargetActor->SetActorLocation(CameraLocation + CameraForward * Distance);
		TargetActor->SetActorRotation(FPPawn->GetCameraRotation());
	}

	// Evita que colisione con nada mientras está flotando frente a cámara.
	TargetActor->SetActorEnableCollision(false);

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
		ReleaseMouseCapture();   // <-- volvemos al modo de cursor libre para click-to-move

		break;

	case EInteractionMode::Examine:
		if (Layer.ExamineTarget)
		{
			Layer.ExamineTarget->SetActorLocation(Layer.ExamineOriginalLocation);
			Layer.ExamineTarget->SetActorRotation(Layer.ExamineOriginalRotation);
			Layer.ExamineTarget->SetActorEnableCollision(true);
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
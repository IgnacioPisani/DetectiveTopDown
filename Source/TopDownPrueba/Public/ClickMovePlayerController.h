#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "FocusTypes.h"
#include "ClickMovePlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UPickableComponent;
class ACameraActor;
// Arriba del archivo, antes de la clase:
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSubSceneStateChanged, bool, bIsEntering);


/**
 * PlayerController del nivel top-down. Maneja:
 *   - Click-to-move normal (NavMesh) cuando no hay nada en el stack de foco.
 *   - Interacción con objetos UPickableComponent.
 *   - Un stack de "capas de foco" (FFocusLayer) que permite anidar estados:
 *     UI sobre sub-escena, examine sobre sub-escena, etc.
 */
UCLASS()
class AClickMovePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AClickMovePlayerController();
	
	// Dentro de la clase, en la sección public:
	UPROPERTY(BlueprintAssignable, Category = "Post Process")
	FOnSubSceneStateChanged OnSubSceneStateChanged;
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// ---------------- Input ----------------
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Click simple: mover personaje / interactuar con un pickeable. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ClickAction;

	/** Mantener presionado el click, usado para arrastrar y rotar en modo Examine. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ClickHoldAction;

	/** Movimiento del mouse (Vector2D), usado en Examine y en la sub-escena. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	/** Escape / botón de cancelar: cierra la capa de foco actual. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> CancelAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MouseLookMappingContext;

	// ---------------- Click-to-move ----------------
	UPROPERTY(EditDefaultsOnly, Category = "ClickToMove")
	float ShortPressThreshold = 0.3f;

	FVector CachedDestination = FVector::ZeroVector;
	float FollowTime = 0.f;

	void OnInputStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();
	
	void OnClickPressed();
	void OnClickHoldStarted();
	void OnClickHoldReleased();
	void OnLook(const FInputActionValue& Value);
	void OnCancel();
	void ApplyPersistentMouseCapture();
	void ReleaseMouseCapture();
	// ------------- Interacción con pickeables -------------
	void HandlePickableInteraction(AActor* Target, UPickableComponent* Pickable);

	// ---------------- Stack de foco ----------------
	UPROPERTY()
	TArray<FFocusLayer> FocusStack;
	bool GetFloorLocationUnderCursor(FVector& OutLocation) const;
	bool bIsDraggingExamine = false;

	void PushShowUI(TSubclassOf<UUserWidget> WidgetClass);
	void PushFirstPersonCamera(ACameraActor* Camera);
	void PushSubScene(TSubclassOf<APawn> PawnClass, AActor* EntryPoint);
	void PushExamine(AActor* TargetActor);

	/** Cierra la capa de foco más reciente y restaura lo anterior. */
	void PopFocus();

	EInteractionMode GetCurrentMode() const;
	bool IsFocused() const { return FocusStack.Num() > 0; }
};
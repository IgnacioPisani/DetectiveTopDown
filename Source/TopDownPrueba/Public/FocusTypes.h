#pragma once

#include "CoreMinimal.h"
#include "PickableTypes.h"
#include "FocusTypes.generated.h"

class UUserWidget;

/**
 * Una "capa" del stack de foco. Cada vez que el jugador entra en un modo de
 * interacción (UI, cámara fija, sub-escena, examine) se apila una de estas;
 * al salir (Escape / botón cerrar) se hace Pop() y se restaura lo que corresponda.
 *
 * Ejemplo de stack para "silla -> escritorio -> objeto en mano":
 *   [0] EnterSubScene  (guarda el pawn de top-down)
 *   [1] Examine        (guarda la rotación original del objeto)
 */
USTRUCT()
struct FFocusLayer
{
	GENERATED_BODY()

	EInteractionMode Mode = EInteractionMode::None;

	/** Pawn que estaba poseído antes de entrar a esta capa. */
	UPROPERTY()
	TObjectPtr<APawn> PreviousPawn = nullptr;

	/** ViewTarget que estaba activo antes de entrar a esta capa. */
	UPROPERTY()
	TObjectPtr<AActor> PreviousViewTarget = nullptr;

	/** Widget creado para esta capa (Mode == ShowUI). */
	UPROPERTY()
	TObjectPtr<UUserWidget> Widget = nullptr;

	/** Pawn spawneado para esta capa (Mode == EnterSubScene). */
	UPROPERTY()
	TObjectPtr<APawn> SpawnedPawn = nullptr;

	/** Actor que se está inspeccionando (Mode == Examine). */
	UPROPERTY()
	TObjectPtr<AActor> ExamineTarget = nullptr;

	/** Rotación original del objeto examinado, para restaurarla al salir. */
	FRotator ExamineOriginalRotation = FRotator::ZeroRotator;
};
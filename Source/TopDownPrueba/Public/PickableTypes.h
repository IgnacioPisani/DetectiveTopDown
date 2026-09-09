#pragma once
 
#include "CoreMinimal.h"
#include "PickableTypes.generated.h"
 
/**
 * Qué pasa cuando el jugador clickea un objeto con UPickableComponent.
 *
 * - None            : el componente existe pero no hace nada (útil como placeholder).
 * - ShowUI          : abre un widget UMG (ej: teléfono, nota, inventario del objeto).
 * - FirstPersonView : blend de cámara a un ACameraActor fijo, sin cambiar de pawn
 *                     (ej: mirar por una cerradura, un cuadro, algo puntual).
 * - SimpleEvent     : dispara un delegate, sin cambiar vista (ej: prender una luz).
 * - EnterSubScene   : posesiona un pawn nuevo dentro de una sub-escena en primera
 *                     persona (ej: la silla que te sienta frente al escritorio).
 * - Examine         : dentro de una sub-escena, permite rotar/inspeccionar el objeto
 *                     arrastrando el mouse (ej: tomar una carta y darla vuelta).
 */
UENUM(BlueprintType)
enum class EInteractionMode : uint8
{
	None,
	ShowUI,
	FirstPersonView,
	SimpleEvent,
	EnterSubScene,
	Examine
};
 
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractSignature);
 
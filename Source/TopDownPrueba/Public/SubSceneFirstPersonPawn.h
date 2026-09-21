#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "SubSceneFirstPersonPawn.generated.h"

class UCameraComponent;

/**
 * Pawn mínimo para sub-escenas en primera persona (ej: sentado frente al
 * escritorio). No se mueve (estilo "Return of the Obra Dinn" / punto de vista
 * fijo); solo permite mirar alrededor con límites de yaw/pitch para no romper
 * la composición de la escena.
 *
 * Si más adelante necesitás que el jugador camine dentro de la sub-escena,
 * este es el lugar para sumar un UFloatingPawnMovement o similar.
 */
UCLASS()
class ASubSceneFirstPersonPawn : public APawn
{
	GENERATED_BODY()

public:
	ASubSceneFirstPersonPawn();
	FVector GetCameraLocation() const { return Camera->GetComponentLocation(); }
	FRotator GetCameraRotation() const { return Camera->GetComponentRotation(); }
	/** Llamado por el PlayerController con el delta del mouse cuando este pawn está activo. */
	void AddLookInput(FVector2D Delta);


protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> PawnRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, Category = "Look")
	float MinPitch = -60.f;

	UPROPERTY(EditAnywhere, Category = "Look")
	float MaxPitch = 60.f;

	/** Rango de giro permitido relativo a la rotación inicial (no absoluto). */
	UPROPERTY(EditAnywhere, Category = "Look")
	float MinYawOffset = -90.f;

	UPROPERTY(EditAnywhere, Category = "Look")
	float MaxYawOffset = 90.f;

private:
	float BaseYaw = 0.f;
};
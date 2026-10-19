#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomCameraPivot.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * Punto fijo de cámara para una habitación. Colocalo en el editor en el
 * lugar donde querés que "viva" la cámara de esa zona, con el SpringArm
 * configurado a la distancia/ángulo que prefieras para esa habitación.
 */
UCLASS()
class ARoomCameraPivot : public AActor
{
	GENERATED_BODY()

public:
	ARoomCameraPivot();

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> PivotRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;
};
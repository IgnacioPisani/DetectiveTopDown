#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomCameraPivot.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class ARoomCameraPivot : public AActor
{
	GENERATED_BODY()

public:
	ARoomCameraPivot();

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	void SetRoomVisible(bool bVisible);
	// Oculta los actores de esta habitación que NO estén también en Keep (puertas/paredes compartidas).
	void HideExcept(const ARoomCameraPivot* Keep);
	bool ContainsActor(AActor* A) const { return RoomActors.Contains(A); }

	// Usadas por el controller en PlayerTick
	UPROPERTY(EditAnywhere, Category = "Room")
	bool bFollowPawn = true;

	UPROPERTY(EditAnywhere, Category = "Room", meta = (EditCondition = "bFollowPawn"))
	float FollowSpeed = 6.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> PivotRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	// Tag que identifica a los actores de esta habitación (Details → Actor → Tags).
	UPROPERTY(EditAnywhere, Category = "Room")
	FName RoomTag;

	// Actores extra opcionales, por si no querés taggearlos.
	UPROPERTY(EditAnywhere, Category = "Room")
	TArray<TObjectPtr<AActor>> ExtraRoomActors;


	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> RoomActors;
};
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomCameraTrigger.generated.h"

class UBoxComponent;
class ARoomCameraPivot;

UCLASS()
class ARoomCameraTrigger : public AActor
{
	GENERATED_BODY()

public:
	ARoomCameraTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, Category = "Room")
	TObjectPtr<ARoomCameraPivot> TargetPivot;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void CheckInitialOverlaps();
};
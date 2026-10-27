#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoopResetTrigger.generated.h"

class UBoxComponent;
class ALoopDirector;

UCLASS()
class ALoopResetTrigger : public AActor
{
	GENERATED_BODY()

public:
	ALoopResetTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, Category = "Loop")
	TObjectPtr<ALoopDirector> Director;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataAsset.h"
#include "LoopDirector.generated.h"

class UPostProcessComponent;
class UAudioComponent;
class ULightComponent;
class USoundBase;
class UMaterialInterface;

UCLASS(BlueprintType)
class ULoopData : public UDataAsset
{
	GENERATED_BODY()

public:
	// Los actores con este tag (ej. "Loop_2") se muestran en este loop; los de otros Loop_* se ocultan.
	UPROPERTY(EditAnywhere, Category = "Loop")
	FName ActiveTag;

	// Se aplica a los actores con tag "LoopLight" (luces Movable).
	UPROPERTY(EditAnywhere, Category = "Loop")
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Loop")
	float LightIntensityMul = 1.f;

	UPROPERTY(EditAnywhere, Category = "Loop")
	TObjectPtr<UMaterialInterface> PostProcessMaterial;

	UPROPERTY(EditAnywhere, Category = "Loop")
	TObjectPtr<USoundBase> Ambience;
};

UCLASS()
class ALoopDirector : public AActor
{
	GENERATED_BODY()

public:
	ALoopDirector();

	UPROPERTY(EditAnywhere, Category = "Loop")
	TArray<TObjectPtr<ULoopData>> Loops;

	// Cualquier actor (un TargetPoint sirve): posición y rotación de reaparición.
	UPROPERTY(EditAnywhere, Category = "Loop")
	TObjectPtr<AActor> RespawnPoint;

	UPROPERTY(EditAnywhere, Category = "Loop")
	bool bWrapAround = false;

	UPROPERTY(EditAnywhere, Category = "Loop")
	float FadeTime = 0.15f;

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void NextLoop();

	// Para eventos "sorpresa" (Level Sequence, sonidos, etc.) desde Blueprint.
	UFUNCTION(BlueprintImplementableEvent, Category = "Loop")
	void OnLoopApplied(int32 Index, ULoopData* Data);

protected:
	virtual void BeginPlay() override;

	void ApplyLoop(int32 Index);
	void DoTeleportAndAdvance();
	void FinishLoop();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPostProcessComponent> PostProcess;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UAudioComponent> AmbienceAudio;

	TArray<TWeakObjectPtr<AActor>> LoopActors;
	TMap<TWeakObjectPtr<ULightComponent>, float> BaseIntensity;
	bool bBusy = false;
};
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

USTRUCT(BlueprintType)
struct FLoopLightSetting
{
	GENERATED_BODY()

	// Tag de la luz en el nivel (Details → Actor → Tags). Puede ser compartido por varias luces.
	UPROPERTY(EditAnywhere, Category = "Loop")
	FName LightTag;

	UPROPERTY(EditAnywhere, Category = "Loop")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Loop", meta = (ClampMin = "0"))
	float IntensityMul = 1.f;
};

UCLASS(BlueprintType)
class ULoopData : public UDataAsset
{
	GENERATED_BODY()

public:
	// Luces que cambian en este loop. Las que no figuran vuelven a su color original.
	UPROPERTY(EditAnywhere, Category = "Loop")
	TArray<FLoopLightSetting> Lights;

	// Actores con alguno de estos tags se OCULTAN en este loop.
	UPROPERTY(EditAnywhere, Category = "Loop")
	TArray<FName> HiddenTags;

	// Actores con alguno de estos tags se MUESTRAN en este loop (útil para assets que arrancan ocultos).
	UPROPERTY(EditAnywhere, Category = "Loop")
	TArray<FName> VisibleTags;

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

	UPROPERTY(EditAnywhere, Category = "Loop")
	TObjectPtr<AActor> RespawnPoint;

	UPROPERTY(EditAnywhere, Category = "Loop")
	bool bWrapAround = false;

	UPROPERTY(EditAnywhere, Category = "Loop")
	float FadeTime = 0.15f;

	UFUNCTION(BlueprintCallable, Category = "Loop")
	void NextLoop();

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

	struct FLightBase
	{
		FLinearColor Color;
		float Intensity;
	};

	TMap<TWeakObjectPtr<ULightComponent>, FLightBase> LightBases;
	TArray<TWeakObjectPtr<AActor>> LoopHiddenActors;
	bool bBusy = false;
};
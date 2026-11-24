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

	// ---------------- Parpadeo ----------------
	UPROPERTY(EditAnywhere, Category = "Loop|Blink")
	bool bBlink = false;

	/** Intensidad "apagada" del parpadeo, relativa a la intensidad ya calculada
	 *  (Base * IntensityMul). 0 = se apaga del todo, 0.3 = atenuada, etc. */
	UPROPERTY(EditAnywhere, Category = "Loop|Blink", meta = (EditCondition = "bBlink", ClampMin = "0", ClampMax = "1"))
	float BlinkLowIntensityMul = 0.1f;

	/** Rango de tiempo entre toggles. Poné Min=Max para un parpadeo parejo,
	 *  o un rango para que se sienta como un foco fallando (más orgánico). */
	UPROPERTY(EditAnywhere, Category = "Loop|Blink", meta = (EditCondition = "bBlink", ClampMin = "0.01"))
	float BlinkIntervalMin = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Loop|Blink", meta = (EditCondition = "bBlink", ClampMin = "0.01"))
	float BlinkIntervalMax = 0.3f;
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
	void Tick(float DeltaTime);
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

	struct FBlinkingLight
	{
		TWeakObjectPtr<ULightComponent> Light;
		float HighIntensity = 0.f;
		float LowIntensity = 0.f;
		float IntervalMin = 0.05f;
		float IntervalMax = 0.3f;
		float TimeRemaining = 0.f;
		bool bHighState = true;
	};

	TArray<FBlinkingLight> BlinkingLights;
	
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
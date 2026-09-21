#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PickableTypes.h"
#include "PickableComponent.generated.h"

class UUserWidget;
class ACameraActor;
class APawn;

/**
 * Agregá este componente a cualquier Actor que quieras que sea clickeable.
 * El modo (Mode) define qué hace el PlayerController cuando lo clickean;
 * solo llenás las propiedades relevantes a ese modo, el resto se ignora.
 */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class UPickableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPickableComponent();

	/** Si está en false, el actor se comporta como no pickeable (ej: objeto ya usado). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bIsPickable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EInteractionMode Mode = EInteractionMode::SimpleEvent;

	// ---------------- ShowUI ----------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|ShowUI",
		meta = (EditCondition = "Mode==EInteractionMode::ShowUI"))
	TSubclassOf<UUserWidget> WidgetClass;

	// ------------- FirstPersonView -------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|FirstPersonView",
		meta = (EditCondition = "Mode==EInteractionMode::FirstPersonView"))
	ACameraActor* FocusCamera = nullptr;

	// -------------- EnterSubScene --------------
	/** Pawn a spawnear/poseer dentro de la sub-escena (ej: BP_DeskFirstPersonPawn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|SubScene",
		meta = (EditCondition = "Mode==EInteractionMode::EnterSubScene"))
	TSubclassOf<APawn> SubScenePawnClass;

	/** Actor cuya Transform define dónde y hacia dónde mira el pawn al entrar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|SubScene",
		meta = (EditCondition = "Mode==EInteractionMode::EnterSubScene"))
	AActor* SubSceneEntryPoint = nullptr;

	// ---------------- Examine ----------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Examine",
		meta = (EditCondition = "Mode==EInteractionMode::Examine"))
	float ExamineRotationSpeed = 0.5f;

	/** Distancia (en unidades) a la que el objeto se ubica frente a la cámara al examinarlo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Examine",
		meta = (EditCondition = "Mode==EInteractionMode::Examine"))
	float ExamineDistance = 150.f;

	// --------------- SimpleEvent ---------------
	/** Bindeá esto en el Blueprint del actor para eventos custom (abrir puerta, etc). */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractSignature OnInteractEvent;
};
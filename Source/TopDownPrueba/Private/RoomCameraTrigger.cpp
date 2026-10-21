#include "RoomCameraTrigger.h"
#include "Components/BoxComponent.h"
#include "RoomCameraPivot.h"
#include "ClickMovePlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

ARoomCameraTrigger::ARoomCameraTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
}

void ARoomCameraTrigger::BeginPlay()
{
	Super::BeginPlay();
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ARoomCameraTrigger::HandleBeginOverlap);

	// Un tick después, para que el pawn ya esté poseído y todos los pivotes hayan hecho BeginPlay.
	GetWorldTimerManager().SetTimerForNextTick(this, &ARoomCameraTrigger::CheckInitialOverlaps);
}

void ARoomCameraTrigger::CheckInitialOverlaps()
{
	if (!TargetPivot) return;

	TArray<AActor*> Overlapping;
	TriggerVolume->GetOverlappingActors(Overlapping, APawn::StaticClass());

	for (AActor* Actor : Overlapping)
	{
		if (APawn* P = Cast<APawn>(Actor))
		{
			if (AClickMovePlayerController* PC = Cast<AClickMovePlayerController>(P->GetController()))
			{
				PC->SetActiveRoomCamera(TargetPivot);
				return;
			}
		}
	}
}

void ARoomCameraTrigger::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!TargetPivot) return;

	APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (!OtherPawn || !OtherPawn->IsPlayerControlled()) return;

	if (AClickMovePlayerController* PC = Cast<AClickMovePlayerController>(OtherPawn->GetController()))
	{
		PC->SetActiveRoomCamera(TargetPivot);
	}
}
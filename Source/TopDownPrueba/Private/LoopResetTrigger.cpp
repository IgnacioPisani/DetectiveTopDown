#include "LoopResetTrigger.h"
#include "LoopDirector.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"

ALoopResetTrigger::ALoopResetTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(100.f, 100.f, 200.f));
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
}

void ALoopResetTrigger::BeginPlay()
{
	Super::BeginPlay();
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ALoopResetTrigger::HandleBeginOverlap);
}

void ALoopResetTrigger::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* P = Cast<APawn>(OtherActor);
	if (Director && P && P->IsPlayerControlled())
	{
		Director->NextLoop();
	}
}
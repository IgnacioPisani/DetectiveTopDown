#include "RoomCameraPivot.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Volume.h"
#include "GameFramework/Info.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "RoomCameraTrigger.h"

ARoomCameraPivot::ARoomCameraPivot()
{
	PrimaryActorTick.bCanEverTick = false;

	PivotRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PivotRoot"));
	SetRootComponent(PivotRoot);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(PivotRoot);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);


}

void ARoomCameraPivot::BeginPlay()
{
	Super::BeginPlay();

	RoomActors.Empty();
	for (AActor* A : ExtraRoomActors)
	{
		if (A) RoomActors.AddUnique(A);
	}

	if (!RoomTag.IsNone())
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			AActor* A = *It;
			if (A != this && A->ActorHasTag(RoomTag))
			{
				RoomActors.AddUnique(A);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: RoomTag está vacío, no se ocultará nada"), *GetNameSafe(this));
	}

	SetRoomVisible(false);
}

void ARoomCameraPivot::SetRoomVisible(bool bVisible)
{
	for (AActor* A : RoomActors)
	{
		if (A) A->SetActorHiddenInGame(!bVisible);
	}
}

void ARoomCameraPivot::HideExcept(const ARoomCameraPivot* Keep)
{
	for (AActor* A : RoomActors)
	{
		if (A && !(Keep && Keep->ContainsActor(A)))
		{
			A->SetActorHiddenInGame(true);
		}
	}
}
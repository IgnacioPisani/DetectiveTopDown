#include "SubSceneFirstPersonPawn.h"
#include "Camera/CameraComponent.h"

ASubSceneFirstPersonPawn::ASubSceneFirstPersonPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	PawnRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PawnRoot"));
	SetRootComponent(PawnRoot);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(PawnRoot);
}

void ASubSceneFirstPersonPawn::BeginPlay()
{
	Super::BeginPlay();
	BaseYaw = GetActorRotation().Yaw;
}

void ASubSceneFirstPersonPawn::AddLookInput(FVector2D Delta)
{
	FRotator NewRotation = GetActorRotation();

	NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch + Delta.Y, MinPitch, MaxPitch);

	const float YawOffset = FMath::UnwindDegrees(NewRotation.Yaw - BaseYaw) + Delta.X;
	NewRotation.Yaw = BaseYaw + FMath::Clamp(YawOffset, MinYawOffset, MaxYawOffset);

	SetActorRotation(NewRotation);
}
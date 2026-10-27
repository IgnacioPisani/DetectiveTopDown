#include "LoopDirector.h"
#include "LoopSubsystem.h"
#include "ClickMovePlayerController.h"
#include "Components/PostProcessComponent.h"
#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"

static const FName LoopHiddenTag(TEXT("LoopHidden"));

ALoopDirector::ALoopDirector()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(Root);
	PostProcess->bUnbound = true;

	AmbienceAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbienceAudio"));
	AmbienceAudio->SetupAttachment(Root);
	AmbienceAudio->bAutoActivate = false;
}

void ALoopDirector::BeginPlay()
{
	Super::BeginPlay();

	// Juntamos actores con cualquier tag "Loop_*" y las luces marcadas.
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* A = *It;
		for (const FName& Tag : A->Tags)
		{
			if (Tag.ToString().StartsWith(TEXT("Loop_")))
			{
				LoopActors.Add(A);
				break;
			}
		}

		if (A->ActorHasTag(TEXT("LoopLight")))
		{
			TArray<ULightComponent*> Lights;
			A->GetComponents<ULightComponent>(Lights);
			for (ULightComponent* L : Lights)
			{
				BaseIntensity.Add(L, L->Intensity);
			}
		}
	}

	if (Loops.Num() > 0)
	{
		const ULoopSubsystem* S = GetGameInstance()->GetSubsystem<ULoopSubsystem>();
		ApplyLoop(FMath::Clamp(S ? S->LoopIndex : 0, 0, Loops.Num() - 1));
	}
}

void ALoopDirector::ApplyLoop(int32 Index)
{
	if (!Loops.IsValidIndex(Index) || !Loops[Index]) return;
	const ULoopData* Data = Loops[Index];

	for (const TWeakObjectPtr<AActor>& Weak : LoopActors)
	{
		AActor* A = Weak.Get();
		if (!A) continue;

		if (A->ActorHasTag(Data->ActiveTag))
		{
			A->Tags.Remove(LoopHiddenTag); // lo vuelve a mostrar el sistema de habitaciones
		}
		else
		{
			A->Tags.AddUnique(LoopHiddenTag);
			A->SetActorHiddenInGame(true);
		}
	}

	for (const TPair<TWeakObjectPtr<ULightComponent>, float>& Pair : BaseIntensity)
	{
		if (ULightComponent* L = Pair.Key.Get())
		{
			L->SetLightColor(Data->LightColor);
			L->SetIntensity(Pair.Value * Data->LightIntensityMul);
		}
	}

	PostProcess->Settings.WeightedBlendables.Array.Reset();
	if (Data->PostProcessMaterial)
	{
		PostProcess->AddOrUpdateBlendable(Data->PostProcessMaterial, 1.f);
	}

	if (Data->Ambience)
	{
		AmbienceAudio->SetSound(Data->Ambience);
		AmbienceAudio->Play();
	}

	// Que la habitación activa muestre los actores del loop nuevo.
	if (AClickMovePlayerController* PC = Cast<AClickMovePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->RefreshRoomVisibility();
	}

	OnLoopApplied(Index, Loops[Index]);
}

void ALoopDirector::NextLoop()
{
	if (bBusy || Loops.Num() == 0) return;
	bBusy = true;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, FadeTime, FLinearColor::Black, false, true);
		}
	}

	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, this, &ALoopDirector::DoTeleportAndAdvance, FadeTime + 0.02f, false);
}

void ALoopDirector::DoTeleportAndAdvance()
{
	ULoopSubsystem* S = GetGameInstance()->GetSubsystem<ULoopSubsystem>();
	if (S)
	{
		int32 Next = S->LoopIndex + 1;
		if (Next >= Loops.Num())
		{
			Next = bWrapAround ? 0 : Loops.Num() - 1;
		}
		S->LoopIndex = Next;
		ApplyLoop(Next);
	}

	if (AClickMovePlayerController* PC = Cast<AClickMovePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->StopMovement();

		if (APawn* Pawn = PC->GetPawn())
		{
			if (RespawnPoint)
			{
				Pawn->TeleportTo(RespawnPoint->GetActorLocation(), RespawnPoint->GetActorRotation());
			}
			if (UPawnMovementComponent* Move = Pawn->GetMovementComponent())
			{
				Move->StopMovementImmediately();
			}
		}

		PC->SnapRoomPivotToPawn();

		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeTime, FLinearColor::Black, false, false);
		}
	}

	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, this, &ALoopDirector::FinishLoop, FadeTime, false);
}

void ALoopDirector::FinishLoop()
{
	bBusy = false;
}
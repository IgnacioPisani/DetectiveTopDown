#include "LoopDirector.h"
#include "LoopSubsystem.h"
#include "ClickMovePlayerController.h"
#include "RoomCameraPivot.h"
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
#include "Engine/World.h"

static const FName LoopHiddenTag(TEXT("LoopHidden"));

// ¿Algún pivote de habitación controla la visibilidad de este actor?
static bool IsRoomManaged(UWorld* World, AActor* A)
{
	for (TActorIterator<ARoomCameraPivot> It(World); It; ++It)
	{
		if (It->ContainsActor(A)) return true;
	}
	return false;
}

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
	UWorld* World = GetWorld();

	// 1) Reset de lo que ocultó el loop anterior
	for (const TWeakObjectPtr<AActor>& Weak : LoopHiddenActors)
	{
		if (AActor* A = Weak.Get())
		{
			A->Tags.Remove(LoopHiddenTag);
			if (!IsRoomManaged(World, A))
			{
				A->SetActorHiddenInGame(false);
			}
		}
	}
	LoopHiddenActors.Reset();

	// 2) Visibles forzados
	for (const FName& Tag : Data->VisibleTags)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(this, Tag, Found);
		for (AActor* A : Found)
		{
			A->Tags.Remove(LoopHiddenTag);
			if (!IsRoomManaged(World, A))
			{
				A->SetActorHiddenInGame(false);
			}
		}
	}

	// 3) Ocultos (ganan sobre visibles)
	for (const FName& Tag : Data->HiddenTags)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(this, Tag, Found);
		for (AActor* A : Found)
		{
			A->Tags.AddUnique(LoopHiddenTag);
			A->SetActorHiddenInGame(true);
			LoopHiddenActors.AddUnique(A);
		}
	}

	// 4) Luces: restaurar originales y aplicar overrides del loop
	for (const TPair<TWeakObjectPtr<ULightComponent>, FLightBase>& Pair : LightBases)
	{
		if (ULightComponent* L = Pair.Key.Get())
		{
			L->SetLightColor(Pair.Value.Color);
			L->SetIntensity(Pair.Value.Intensity);
		}
	}

	for (const FLoopLightSetting& Setting : Data->Lights)
	{
		if (Setting.LightTag.IsNone()) continue;

		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(this, Setting.LightTag, Found);
		for (AActor* A : Found)
		{
			TArray<ULightComponent*> Lights;
			A->GetComponents<ULightComponent>(Lights);
			for (ULightComponent* L : Lights)
			{
				const TWeakObjectPtr<ULightComponent> Key(L);
				if (!LightBases.Contains(Key))
				{
					LightBases.Add(Key, FLightBase{ L->GetLightColor(), L->Intensity });
				}
				const FLightBase& Base = LightBases[Key];

				L->SetLightColor(Setting.Color);
				L->SetIntensity(Base.Intensity * Setting.IntensityMul);
			}
		}
	}

	// 5) Post process, audio, visibilidad de habitación
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
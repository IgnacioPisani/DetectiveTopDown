
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

// Algun pivote de habitacion controla la visibilidad de este actor?
static bool IsRoomManaged(UWorld* World, AActor* A)
{
    for (TActorIterator<ARoomCameraPivot> It(World); It; ++It)
    {
        if (It->ContainsActor(A))
        {
            return true;
        }
    }

    return false;
}

ALoopDirector::ALoopDirector()
{
    PrimaryActorTick.bCanEverTick = true;

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

    UE_LOG(LogTemp, Warning, TEXT("[LOOP] LoopDirector BeginPlay"));

    if (Loops.Num() > 0)
    {
        const ULoopSubsystem* S = GetGameInstance()->GetSubsystem<ULoopSubsystem>();

        int32 InitialIndex = FMath::Clamp(
            S ? S->LoopIndex : 0,
            0,
            Loops.Num() - 1
        );

        ApplyLoop(InitialIndex);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[LOOP] No hay Loops configurados"));
    }
}

void ALoopDirector::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    for (int32 i = BlinkingLights.Num() - 1; i >= 0; --i)
    {
        FBlinkingLight& Entry = BlinkingLights[i];

        ULightComponent* L = Entry.Light.Get();

        if (!L)
        {
            UE_LOG(LogTemp, Warning, TEXT("[BLINK] Luz invalida, eliminando entrada"));

            BlinkingLights.RemoveAtSwap(i);
            continue;
        }

        Entry.TimeRemaining -= DeltaTime;

        if (Entry.TimeRemaining <= 0.f)
        {
            Entry.bHighState = !Entry.bHighState;

            float NewIntensity = Entry.bHighState
                ? Entry.HighIntensity
                : Entry.LowIntensity;

            L->SetIntensity(NewIntensity);

            UE_LOG(LogTemp, Warning,
                TEXT("[BLINK] %s | Intensidad: %.2f | Estado: %s"),
                *GetNameSafe(L),
                NewIntensity,
                Entry.bHighState ? TEXT("ON") : TEXT("OFF"));

            Entry.TimeRemaining = FMath::RandRange(
                Entry.IntervalMin,
                Entry.IntervalMax
            );
        }
    }
}

void ALoopDirector::ApplyLoop(int32 Index)
{
    if (!Loops.IsValidIndex(Index) || !Loops[Index])
    {
        UE_LOG(LogTemp, Error, TEXT("[LOOP] Index invalido: %d"), Index);
        return;
    }

    const ULoopData* Data = Loops[Index];
    UWorld* World = GetWorld();

    BlinkingLights.Reset();

    UE_LOG(LogTemp, Warning, TEXT("========== APPLY LOOP =========="));
    UE_LOG(LogTemp, Warning, TEXT("[LOOP] Index: %d"), Index);
    UE_LOG(LogTemp, Warning, TEXT("[LOOP] Luces configuradas: %d"), Data->Lights.Num());
    UE_LOG(LogTemp, Warning, TEXT("[LOOP] LightBases antes: %d"), LightBases.Num());

    // 1. Restaurar actores ocultos por el loop anterior
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

    // 2. Actores que deben mostrarse
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

    // 3. Actores que deben ocultarse
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

    // 4. Restaurar las luces a sus valores originales
    for (const TPair<TWeakObjectPtr<ULightComponent>, FLightBase>& Pair : LightBases)
    {
        if (ULightComponent* L = Pair.Key.Get())
        {
            L->SetLightColor(Pair.Value.Color);
            L->SetIntensity(Pair.Value.Intensity);
        }
    }

    // 5. Buscar y configurar todas las luces del loop
    for (const FLoopLightSetting& Setting : Data->Lights)
    {
        if (Setting.LightTag.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("[LOOP] LightTag vacio, se omite"));
            continue;
        }

        TArray<AActor*> Found;

        UGameplayStatics::GetAllActorsWithTag(this, Setting.LightTag, Found);

        UE_LOG(LogTemp, Warning,
            TEXT("[LOOP] Buscando Tag: %s | Actores encontrados: %d"),
            *Setting.LightTag.ToString(),
            Found.Num());

        for (AActor* A : Found)
        {
            TArray<ULightComponent*> Lights;

            A->GetComponents<ULightComponent>(Lights);

            UE_LOG(LogTemp, Warning,
                TEXT("[LOOP] Actor: %s | Componentes de luz: %d"),
                *GetNameSafe(A),
                Lights.Num());

            for (ULightComponent* L : Lights)
            {
                if (!L) continue;

                const TWeakObjectPtr<ULightComponent> Key(L);

                // Guardar valores originales una sola vez
                if (!LightBases.Contains(Key))
                {
                    LightBases.Add(
                        Key,
                        FLightBase{ L->GetLightColor(), L->Intensity }
                    );
                }

                const FLightBase& Base = LightBases[Key];

                float HighIntensity = Base.Intensity * Setting.IntensityMul;
                float LowIntensity = HighIntensity * Setting.BlinkLowIntensityMul;

                L->SetLightColor(Setting.Color);

                UE_LOG(LogTemp, Warning,
                    TEXT("[LOOP] Luz: %s | Base: %.2f | High: %.2f | Blink: %s"),
                    *GetNameSafe(L),
                    Base.Intensity,
                    HighIntensity,
                    Setting.bBlink ? TEXT("TRUE") : TEXT("FALSE"));

                if (Setting.bBlink)
                {
                    L->SetIntensity(HighIntensity);

                    FBlinkingLight& Entry = BlinkingLights.AddDefaulted_GetRef();

                    Entry.Light = L;
                    Entry.HighIntensity = HighIntensity;
                    Entry.LowIntensity = LowIntensity;
                    Entry.IntervalMin = FMath::Max(0.01f, Setting.BlinkIntervalMin);
                    Entry.IntervalMax = FMath::Max(Entry.IntervalMin, Setting.BlinkIntervalMax);
                    Entry.TimeRemaining = FMath::RandRange(
                        Entry.IntervalMin,
                        Entry.IntervalMax
                    );
                    Entry.bHighState = true;

                    UE_LOG(LogTemp, Warning,
                        TEXT("[BLINK] REGISTRADA: %s | High: %.2f | Low: %.2f | Intervalo: %.2f - %.2f"),
                        *GetNameSafe(L),
                        Entry.HighIntensity,
                        Entry.LowIntensity,
                        Entry.IntervalMin,
                        Entry.IntervalMax);
                }
                else
                {
                    L->SetIntensity(HighIntensity);
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[BLINK] Total de luces registradas: %d"),
        BlinkingLights.Num());

    UE_LOG(LogTemp, Warning,
        TEXT("[LOOP] LightBases despues: %d"),
        LightBases.Num());

    // 6. Post process
    PostProcess->Settings.WeightedBlendables.Array.Reset();

    if (Data->PostProcessMaterial)
    {
        PostProcess->AddOrUpdateBlendable(Data->PostProcessMaterial, 1.f);
    }

    // 7. Audio ambiente
    if (Data->Ambience)
    {
        AmbienceAudio->SetSound(Data->Ambience);
        AmbienceAudio->Play();
    }
    else
    {
        AmbienceAudio->Stop();
    }

    // 8. Actualizar visibilidad de habitaciones
    if (AClickMovePlayerController* PC =
        Cast<AClickMovePlayerController>(
            UGameplayStatics::GetPlayerController(this, 0)))
    {
        PC->RefreshRoomVisibility();
    }

    OnLoopApplied(Index, Loops[Index]);

    UE_LOG(LogTemp, Warning, TEXT("========== FIN APPLY LOOP =========="));
}

void ALoopDirector::NextLoop()
{
    if (bBusy || Loops.Num() == 0) return;

    bBusy = true;

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (PC->PlayerCameraManager)
        {
            PC->PlayerCameraManager->StartCameraFade(
                0.f, 1.f, FadeTime, FLinearColor::Black, false, true
            );
        }
    }

    FTimerHandle H;

    GetWorldTimerManager().SetTimer(
        H,
        this,
        &ALoopDirector::DoTeleportAndAdvance,
        FadeTime + 0.02f,
        false
    );
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

    if (AClickMovePlayerController* PC =
        Cast<AClickMovePlayerController>(
            UGameplayStatics::GetPlayerController(this, 0)))
    {
        PC->StopMovement();

        if (APawn* Pawn = PC->GetPawn())
        {
            if (RespawnPoint)
            {
                Pawn->TeleportTo(
                    RespawnPoint->GetActorLocation(),
                    RespawnPoint->GetActorRotation()
                );
            }

            if (UPawnMovementComponent* Move = Pawn->GetMovementComponent())
            {
                Move->StopMovementImmediately();
            }
        }

        PC->SnapRoomPivotToPawn();

        if (PC->PlayerCameraManager)
        {
            PC->PlayerCameraManager->StartCameraFade(
                1.f, 0.f, FadeTime, FLinearColor::Black, false, false
            );
        }
    }

    FTimerHandle H;

    GetWorldTimerManager().SetTimer(
        H,
        this,
        &ALoopDirector::FinishLoop,
        FadeTime,
        false
    );
}

void ALoopDirector::FinishLoop()
{
    bBusy = false;
}

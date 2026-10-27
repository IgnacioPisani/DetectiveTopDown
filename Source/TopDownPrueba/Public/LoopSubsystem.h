#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LoopSubsystem.generated.h"

UCLASS()
class ULoopSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Loop")
	int32 LoopIndex = 0;
};
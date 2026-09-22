#include "PickableComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"

UPickableComponent::UPickableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}



void UPickableComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	Owner->GetComponents<UMeshComponent>(CachedMeshComponents);

	Owner->OnBeginCursorOver.AddDynamic(this, &UPickableComponent::HandleBeginCursorOver);
	Owner->OnEndCursorOver.AddDynamic(this, &UPickableComponent::HandleEndCursorOver);
}

void UPickableComponent::HandleBeginCursorOver(AActor* TouchedActor)
{
	if (!bIsPickable || !bEnableHoverHighlight || !HoverHighlightMaterial) return;

	for (UMeshComponent* Mesh : CachedMeshComponents)
	{
		if (Mesh)
		{
			Mesh->SetOverlayMaterial(HoverHighlightMaterial);
		}
	}
}

void UPickableComponent::HandleEndCursorOver(AActor* TouchedActor)
{
	for (UMeshComponent* Mesh : CachedMeshComponents)
	{
		if (Mesh)
		{
			Mesh->SetOverlayMaterial(nullptr);
		}
	}
}
// Copyright 2020, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "Viz/SLVizHighlightMarkerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values for this component's properties
ASLVizHighlightMarkerManager::ASLVizHighlightMarkerManager()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SL_HighlightMarkerManagerRoot"));

#if WITH_EDITORONLY_DATA
	// Make manager sprite smaller (used to easily find the actor in the world)
	SpriteScale = 0.35;
#endif // WITH_EDITORONLY_DATA
}

// Called when actor removed from game or game ended
void ASLVizHighlightMarkerManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	ClearAllMarkers();
}

// Clear hihlight marker
void ASLVizHighlightMarkerManager::ClearMarker(USLVizHighlightMarker* HighlightMarker)
{
	if (HighlightMarker->IsValidLowLevel())
	{
		//HighlightMarker->DestroyComponent();
		HighlightMarker->ConditionalBeginDestroy();
	}
	HighlightMarkers.Remove(HighlightMarker);
}

// Clear all markers
void ASLVizHighlightMarkerManager::ClearAllMarkers()
{
	for (const auto& HM : HighlightMarkers)
	{
		if (HM->IsValidLowLevel())
		{
			//HM->DestroyComponent();
			HM->ConditionalBeginDestroy();
		}
	}
	HighlightMarkers.Empty();
}

/* Highlight markers */
// Create a highlight marker for the given static mesh component
USLVizHighlightMarker* ASLVizHighlightMarkerManager::CreateHighlightMarker(UStaticMeshComponent* SMC, const FLinearColor& Color, ESLVizHighlightMarkerType Type)
{
	USLVizHighlightMarker* HighlightMarker = CreateNewHighlightMarker();
	HighlightMarker->Init(SMC, Color, Type);
	return HighlightMarker;
}

// Create a highlight marker for the given skeletal mesh component
USLVizHighlightMarker* ASLVizHighlightMarkerManager::CreateHighlightMarker(USkeletalMeshComponent* SkMC, const FLinearColor& Color, ESLVizHighlightMarkerType Type)
{
	USLVizHighlightMarker* HighlightMarker = CreateNewHighlightMarker();
	HighlightMarker->Init(SkMC, Color, Type);
	return HighlightMarker;
}

// Create a highlight marker for the given bone (material index) skeletal mesh component
USLVizHighlightMarker* ASLVizHighlightMarkerManager::CreateHighlightMarker(USkeletalMeshComponent* SkMC, int32 MaterialIndex, const FLinearColor& Color, ESLVizHighlightMarkerType Type)
{
	USLVizHighlightMarker* HighlightMarker = CreateNewHighlightMarker();
	HighlightMarker->Init(SkMC, MaterialIndex, Color, Type);
	return HighlightMarker;
}

// Create a highlight marker for the given bones (material indexes) skeletal mesh component
USLVizHighlightMarker * ASLVizHighlightMarkerManager::CreateHighlightMarker(USkeletalMeshComponent* SkMC, TArray<int32>& MaterialIndexes, const FLinearColor& Color, ESLVizHighlightMarkerType Type)
{
	USLVizHighlightMarker* HighlightMarker = CreateNewHighlightMarker();
	HighlightMarker->Init(SkMC, MaterialIndexes, Color, Type);
	return HighlightMarker;
}


// Create and register the highlight marker
USLVizHighlightMarker* ASLVizHighlightMarkerManager::CreateNewHighlightMarker()
{
	USLVizHighlightMarker* HighlightMarker = NewObject<USLVizHighlightMarker>(this);
	HighlightMarker->RegisterComponent();
	AddInstanceComponent(HighlightMarker); // Makes it appear in the editor
	//AddOwnedComponent(Marker);
	HighlightMarker->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	HighlightMarkers.Emplace(HighlightMarker);
	return HighlightMarker;
}
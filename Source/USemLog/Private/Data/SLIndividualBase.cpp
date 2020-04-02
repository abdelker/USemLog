// Copyright 2017-2020, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "Data/SLIndividualBase.h"

// Set the semantic owner actor
void USLIndividualBase::PostInitProperties()
{
	Super::PostInitProperties();

	// First outer is the component, second the actor
	if (AActor* Owner = Cast<AActor>(GetOuter()->GetOuter()))
	{
		SetSemOwner(Owner);
	}
}

// Save data to owners tag
bool USLIndividualBase::SaveToTag(bool bOverwrite)
{
	if (!SemanticOwner)
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d Owner not set, cannot write to tags.."), *FString(__FUNCTION__), __LINE__);
		return false;
	}
	return true;
}

// Load data from owners tag
bool USLIndividualBase::LoadFromTag(bool bOverwrite)
{
	if (!SemanticOwner)
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d Owner not set, cannot read from tags.."), *FString(__FUNCTION__), __LINE__);
		return false;
	}
	return true;
}

// All properties are set for runtime
bool USLIndividualBase::IsRuntimeReady() const
{
	return SemanticOwner != nullptr;
}
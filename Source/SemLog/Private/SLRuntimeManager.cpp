// Copyright 2017, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "SLRuntimeManager.h"
#include "SLUtils.h"

// Sets default values
ASLRuntimeManager::ASLRuntimeManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// Defaults
	LogDirectory = FPaths::GameDir() + "SemLog";
	EpisodeId = "AutoGenerated";
	
	bLogRawData = true;
	RawDataUpdateRate = 0.f;
	TimePassedSinceLastUpdate = 0.f;
	bWriteRawDataToFile = true;
	bBroadcastRawData = false;
	
	bLogEventData = true;
	bWriteEventDataToFile = true;
	bBroadcastEventData = false;
}

// Called when the game starts or when spawned
void ASLRuntimeManager::BeginPlay()
{
	Super::BeginPlay();

	// No tick by default
	SetActorTickEnabled(false);

	// Generate episode Id if not manually entered
	if (EpisodeId.Equals("AutoGenerated"))
	{
		EpisodeId = FSLUtils::GenerateRandomFString(4);
	}
	
	// Setup raw data logger
	if (bLogRawData)
	{
		// Create raw data logger UObject
		RawDataLogger = NewObject<USLRawDataLogger>(this, TEXT("RawDataLogger"));		
		
		// Init logger 
		RawDataLogger->Init(GetWorld(), 0.1f);

		// Set logging type
		if (bWriteRawDataToFile)
		{
			RawDataLogger->InitFileHandle(EpisodeId, LogDirectory);
		}

		if (bBroadcastRawData)
		{
			RawDataLogger->InitBroadcaster();
		}

		// Log the first entry (static and dynamic entities)
		RawDataLogger->LogFirstEntry();

		// Enable tick for raw data logging
		SetActorTickEnabled(true);
	}
	
	// Setup event data logger
	if (bLogEventData)
	{
		// Create event data logger UObject
		EventDataLogger = NewObject<USLEventDataLogger>(this, TEXT("EventDataLogger"));

		// Initialize the event data
		EventDataLogger->InitLogger(EpisodeId);

		// Start logger
		EventDataLogger->StartLogger(GetWorld()->GetTimeSeconds());
	}
}

// Called when actor removed from game or game ended
void ASLRuntimeManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if(bLogEventData && EventDataLogger)
	{
		// Finish up the logger - Terminate idle events
		EventDataLogger->FinishLogger(GetWorld()->GetTimeSeconds());

		if (bWriteEventDataToFile)
		{
			EventDataLogger->WriteEventsToFile(LogDirectory);
		}

		if (bBroadcastEventData)
		{
			EventDataLogger->BroadcastFinishedEvents();
		}
	}
}

// Called every frame
void ASLRuntimeManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Increase duration
	TimePassedSinceLastUpdate += DeltaTime;

	if (RawDataUpdateRate < TimePassedSinceLastUpdate)
	{
		// Log the raw data of the dynamic entities
		RawDataLogger->LogDynamicEntities();
		TimePassedSinceLastUpdate = 0.f;
	}
}

// Add finished event
bool ASLRuntimeManager::AddFinishedEvent(
	const FString Namespace,
	const FString Class,
	const FString Id,
	const float StartTime,
	const float EndTime,
	const TArray<FOwlTriple>& Properties)
{
	return ASLRuntimeManager::AddFinishedEvent("&" + Namespace + ";" + Class + "_" + Id, StartTime, EndTime, Properties);
}

// Add finished event, Name = Class + Id
bool ASLRuntimeManager::AddFinishedEvent(
	const FString Namespace,
	const FString Name,
	const float StartTime,
	const float EndTime,
	const TArray<FOwlTriple>& Properties)
{
	return ASLRuntimeManager::AddFinishedEvent("&" + Namespace + ";" + Name, StartTime, EndTime, Properties);
}

// Add finished event, FullName = Namespace + Class + Id
bool ASLRuntimeManager::AddFinishedEvent(
	const FString FullName,
	const float StartTime,
	const float EndTime,
	const TArray<FOwlTriple>& Properties)
{
	if (bLogEventData && EventDataLogger && EventDataLogger->IsStarted())
	{		
		EventDataLogger->InsertFinishedEvent();
		return true;
	}
	return false;
}

// Start an event
bool ASLRuntimeManager::StartEvent(
	const FString Namespace,
	const FString Class,
	const FString Id,
	const float StartTime,
	const TArray<FOwlTriple>& Properties)
{
	return ASLRuntimeManager::StartEvent("&" + Namespace + ";" + Class + "_" + Id, StartTime, Properties);
}

// Start an event, Name = Class + Id
bool ASLRuntimeManager::StartEvent(
	const FString Namespace,
	const FString Name,
	const float StartTime,
	const TArray<FOwlTriple>& Properties)
{
	return ASLRuntimeManager::StartEvent("&" + Namespace + ";" + Name, StartTime, Properties);
}

// Start an event, FullName = Namespace + Class + Id
bool ASLRuntimeManager::StartEvent(
	const FString FullName,
	const float StartTime,
	const TArray<FOwlTriple>& Properties)
{
	if (bLogEventData && EventDataLogger && EventDataLogger->IsStarted())
	{
		EventDataLogger->StartAnEvent();
		return true;
	}
	return false;
}

// Start an event
bool ASLRuntimeManager::FinishEvent(
	const FString Namespace,
	const FString Class,
	const FString Id,
	const float EndTime,
	const TArray<FOwlTriple>& Properties) 
{
	return ASLRuntimeManager::FinishEvent("&" + Namespace + ";" + Class + "_" + Id, EndTime, Properties);
}

// Start an event, Name = Class + Id
bool ASLRuntimeManager::FinishEvent(
	const FString Namespace,
	const FString Name,
	const float EndTime,
	const TArray<FOwlTriple>& Properties)
{
	return ASLRuntimeManager::FinishEvent("&" + Namespace + ";" + Name, EndTime, Properties);
}

// Start an event, FullName = Namespace + Class + Id
bool ASLRuntimeManager::FinishEvent(
	const FString FullName,
	const float EndTime,
	const TArray<FOwlTriple>& Properties)
{
	if (bLogEventData && EventDataLogger && EventDataLogger->IsStarted())
	{
		EventDataLogger->FinishAnEvent();
		return true;
	}
	return false;
}

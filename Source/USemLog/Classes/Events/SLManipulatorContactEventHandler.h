// Copyright 2017-2019, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#pragma once

#include "Events/ISLEventHandler.h"
#include "Events/SLContactEvent.h"
#include "TimerManager.h"

// Forward declarations
struct FSLContactResult;

/**
 * Listens to contact events input, and outputs finished semantic contact events
 */
class FSLManipulatorContactEventHandler : public ISLEventHandler
{
public:
	// Init parent
	void Init(UObject* InParent) override;

	// Start listening to input
	void Start() override;

	// Terminate listener, finish and publish remaining events
	void Finish(float EndTime, bool bForced = false) override;

private:
	// Start new contact event
	void AddNewEvent(const FSLContactResult& InResult);

	// Finish then publish the event
	bool FinishEvent(const FSLEntity&, float EndTime);

	// Terminate and publish started events (this usually is called at end play)
	void FinishAllEvents(float EndTime);

	// Event called when a semantic overlap event begins
	void OnSLOverlapBegin(const FSLContactResult& InResult);

	// Event called when a semantic overlap event ends
	void OnSLOverlapEnd(const FSLEntity& Self, const FSLEntity& Other, float Time);

	private:
	// Parent semantic overlap area
	class USLManipulatorListener* Parent = nullptr;

	// Array of started contact events
	TArray<TSharedPtr<FSLContactEvent>> StartedEvents;
};

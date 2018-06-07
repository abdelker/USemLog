// Copyright 2018, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#pragma once

#include "EventData/IEvent.h"

/**
* Dummy event class
*/

class FDummyEvent : public IEvent
{
public:
	// Default constructor
	FDummyEvent();

	/* Begin IEvent interface */
	// To an owl representation of the event
	virtual FOwlNode ToOwlNode() const override;
};
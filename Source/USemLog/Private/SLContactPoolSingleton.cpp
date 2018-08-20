// Copyright 2018, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "SLContactPoolSingleton.h"
#include "SLContactListener.h"

//static TSharedPtr<USLContactPoolSingleton> USLContactPoolSingleton::StaticInstance = nullptr;
static USLContactPoolSingleton* StaticInstance = nullptr;

// Constructor
USLContactPoolSingleton::USLContactPoolSingleton()
{
	UE_LOG(LogTemp, Error, TEXT("[%s][%d]"), TEXT(__FUNCTION__), __LINE__);
	bNewData = false;
}

// Destructor
USLContactPoolSingleton::~USLContactPoolSingleton()
{
	UE_LOG(LogTemp, Error, TEXT("[%s][%d]"), TEXT(__FUNCTION__), __LINE__);
}

// Get singleton
USLContactPoolSingleton* USLContactPoolSingleton::GetInstance()
{
	UE_LOG(LogTemp, Error, TEXT("[%s][%d]"), TEXT(__FUNCTION__), __LINE__);
	if (!StaticInstance)
	{
		StaticInstance = NewObject<USLContactPoolSingleton>();
		StaticInstance->AddToRoot(); // Avoid garbage collection
	}
	return StaticInstance;
}

// Delete instance
void USLContactPoolSingleton::DeleteInstance()
{
	StaticInstance->ConditionalBeginDestroy();
}

/** Begin FTickableGameObject interface */
// Called after ticking all actors, DeltaTime is the time passed since the last call.
void USLContactPoolSingleton::Tick(float DeltaTime)
{
	if (bNewData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s][%d]"), TEXT(__FUNCTION__), __LINE__);
		bNewData = false;
	}
}

//// Return if object is ready to be ticked
//bool USLContactPoolSingleton::IsTickable() const
//{
//	return bIsTickable;
//}

// Return the stat id to use for this tickable
TStatId USLContactPoolSingleton::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USLRawDataLogger, STATGROUP_Tickables);
}
/** End FTickableGameObject interface */


void USLContactPoolSingleton::Foo()
{
	UE_LOG(LogTemp, Error, TEXT("[%s][%d]"), TEXT(__FUNCTION__), __LINE__);
}

void USLContactPoolSingleton::Register(USLContactListener* ContactListener)
{
	UE_LOG(LogTemp, Error, TEXT("[%s][%d] Registering: %s"),
		TEXT(__FUNCTION__), __LINE__, *ContactListener->GetName());

	ContactListener->OnBeginSemanticContact.BindUObject(this, &USLContactPoolSingleton::OnNewBeginSemanticContact);
}

void USLContactPoolSingleton::OnNewBeginSemanticContact(AActor* OtherActor, const FString& Id)
{
	UE_LOG(LogTemp, Error, TEXT("[%s][%d] OnNewBeginSemanticContact: %s"),
		TEXT(__FUNCTION__), __LINE__, *OtherActor->GetName());
	bNewData = true;
}

void USLContactPoolSingleton::OnNewEndSemanticContact(AActor* OtherActor, const FString& Id)
{
	UE_LOG(LogTemp, Error, TEXT("[%s][%d] OnNewBeginSemanticContact: %s"),
		TEXT(__FUNCTION__), __LINE__, *OtherActor->GetName());
	bNewData = true;
}
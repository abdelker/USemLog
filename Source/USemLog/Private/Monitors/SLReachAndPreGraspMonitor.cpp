// Copyright 2019, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "Monitors/SLReachAndPreGraspMonitor.h"
#include "Monitors/SLMonitorStructs.h"
#include "Monitors/SLManipulatorMonitor.h"
#include "Individuals/SLIndividualComponent.h"
#include "Individuals/SLIndividualUtils.h"
#include "Individuals/Type/SLBaseIndividual.h"

#include "Animation/SkeletalMeshActor.h"
#include "Engine/StaticMeshActor.h"
#include "TimerManager.h"
#include "Components/StaticMeshComponent.h"


// Set default values
USLReachAndPreGraspMonitor::USLReachAndPreGraspMonitor()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	InitSphereRadius(30.f);

	bIsInit = false;
	bIsStarted = false;
	bIsFinished = false;

	CurrGraspedIndividual = nullptr;

	// Default values
	UpdateRate = 0.027;
	
	ShapeColor = FColor::Orange.WithAlpha(64);
}

// Dtor
USLReachAndPreGraspMonitor::~USLReachAndPreGraspMonitor()
{
	if (!bIsFinished)
	{
		Finish(true);
	}
}

// Called every frame, used for timeline visualizations, activated and deactivated on request
void USLReachAndPreGraspMonitor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateCandidatesData();
}

// Initialize trigger areas for runtime, check if owner is valid and semantically annotated
bool USLReachAndPreGraspMonitor::Init()
{
	if (bIgnore)
	{
		return false;
	}

	if (!bIsInit)
	{
		// Make sure the owner is semantically annotated
		if(USLIndividualComponent* IC = FSLIndividualUtils::GetIndividualComponent(GetOwner()))
		{
			OwnerIndividualComponent = IC;
			if (!OwnerIndividualComponent->IsLoaded())
			{				
				UE_LOG(LogTemp, Error, TEXT("%s::%d %s's individual component is not loaded.."), *FString(__FUNCTION__), __LINE__, *GetOwner()->GetName());
				return false;
			}

			// Set the individual object
			OwnerIndividualObject = OwnerIndividualComponent->GetIndividualObject();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s::%d %s has no individual component.."), *FString(__FUNCTION__), __LINE__, *GetOwner()->GetName());
			return false;
		}

		// Set tick update rate
		if (UpdateRate > 0.f)
		{
			SetComponentTickInterval(UpdateRate);
		}
		
		// Disable overlaps until start
		SetGenerateOverlapEvents(false);

		// Subscribe for grasp notifications from sibling monitor component
		if(SubscribeForManipulatorEvents())
		{
			bIsInit = true;
			return true;
		}
	}
	return false;
}

// Start listening to grasp events, update currently overlapping objects
void USLReachAndPreGraspMonitor::Start()
{
	if (!bIsStarted && bIsInit)
	{
		// Start listening for overlaps
		SetGenerateOverlapEvents(true);

		// Iterate through the currently overlapping componets
		TriggerInitialOverlaps();

		// Bind overlap events
		OnComponentBeginOverlap.AddDynamic(this, &USLReachAndPreGraspMonitor::OnOverlapBegin);
		OnComponentEndOverlap.AddDynamic(this, &USLReachAndPreGraspMonitor::OnOverlapEnd);
		
		// Start candidate update callback
		SetComponentTickEnabled(true);
		
		// Mark as started
		bIsStarted = true;
	}
}

// Stop publishing grasp events
void USLReachAndPreGraspMonitor::Finish(bool bForced)
{
	if (!bIsFinished && (bIsInit || bIsStarted))
	{
		OnComponentBeginOverlap.RemoveAll(this);
		OnComponentEndOverlap.RemoveAll(this);
		SetComponentTickEnabled(false);
		
		// Mark as finished
		bIsStarted = false;
		bIsInit = false;
		bIsFinished = true;
	}
}

#if WITH_EDITOR
// Called after the CanidateData++ constructor and after the properties have been initialized
void USLReachAndPreGraspMonitor::PostInitProperties()
{
	Super::PostInitProperties();
	RelocateSphere();
}

// Called when a property is changed in the editor
void USLReachAndPreGraspMonitor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Get the changed property name
	const FName PropertyName = (PropertyChangedEvent.Property != NULL) ?
		PropertyChangedEvent.Property->GetFName() : NAME_None;

	// Set pre-defined parameters
	if (PropertyName == GET_MEMBER_NAME_CHECKED(USLReachAndPreGraspMonitor, SphereRadius))
	{
		RelocateSphere();
	}
}

// Move the sphere location so that its surface overlaps with the end of the manipulator
void USLReachAndPreGraspMonitor::RelocateSphere()
{
	if (GetOwner())
	{
		USceneComponent* RootComp = GetOwner()->GetRootComponent();
		//FVector Center = GetOwner()->CalculateComponentsBoundingBoxInLocalSpace().GetCenter();
		const float BoundsCenterOffsetDist = FVector::Distance(RootComp->Bounds.Origin, RootComp->GetComponentLocation());
		float OwnerRadius;
		float OwnerHalfHeight;
		GetOwner()->GetRootComponent()->CalcBoundingCylinder(OwnerRadius, OwnerHalfHeight);
		if (OwnerRadius < SphereRadius)
		{
			SetRelativeLocation(FVector(SphereRadius - OwnerRadius + BoundsCenterOffsetDist, 0.f, 0.f));
		}
	}
}
#endif // WITH_EDITOR

// Subscribe for grasp events from sibling component (contacts with hand and grasp events)
bool USLReachAndPreGraspMonitor::SubscribeForManipulatorEvents()
{
	if(USLManipulatorMonitor* ManipulatorMonitor = CastChecked<USLManipulatorMonitor>(
		GetOwner()->GetComponentByClass(USLManipulatorMonitor::StaticClass())))
	{
		// Timeline reaching   ,    pre grasp
		// [-----------contact][contact--------grasp]
		ManipulatorMonitor->OnBeginManipulatorContact.AddUObject(this, &USLReachAndPreGraspMonitor::OnSLManipulatorContactBegin);
		ManipulatorMonitor->OnEndManipulatorContact.AddUObject(this, &USLReachAndPreGraspMonitor::OnSLManipulatorContactEnd);
		ManipulatorMonitor->OnBeginManipulatorGrasp.AddUObject(this, &USLReachAndPreGraspMonitor::OnSLGraspBegin);
		ManipulatorMonitor->OnEndManipulatorGrasp.AddUObject(this, &USLReachAndPreGraspMonitor::OnSLGraspEnd);
		return true;
	}
	return false;
}

// Update callback, checks distance to hand, if it increases it resets the start time
void USLReachAndPreGraspMonitor::UpdateCandidatesData()
{
	const float CurrTimestamp = GetWorld()->GetTimeSeconds();
	for (auto& CanidateData : CandidatesData)
	{
		const float CurrDist = FVector::Distance(GetOwner()->GetActorLocation(), CanidateData.Key->GetParentActor()->GetActorLocation());
		const float PrevDist = CanidateData.Value.Get<1>();
		const float DiffDist = PrevDist - CurrDist;

		// Ignore small difference changes (MinDist)
		if (DiffDist > MinDist)
		{
			// Positive difference makes the hand closer to the object, update the distance
			CanidateData.Value.Get<ESLTimeAndDist::SLDist>() = CurrDist;

			//UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f] %s is moving closer to %s; PrevDist=%f; CurrDist=%f; DiffDist=%f; [%s - %s];"),
			//	*FString(__func__), __LINE__, 
			//	CurrTimestamp, 
			//	*SemanticOwner.Obj->GetName(), 
			//	*CanidateData.Key->GetName(),
			//	PrevDist, CurrDist, DiffDist,
			//	*GetOwner()->GetActorLocation().ToString(),
			//	*CanidateData.Key->GetActorLocation().ToString());
		}
		else if (DiffDist < -MinDist)
		{
			// Negative difference makes the hand further away from the object, update distance, reset the start time
			CanidateData.Value.Get<ESLTimeAndDist::SLTime>() = CurrTimestamp;
			CanidateData.Value.Get<ESLTimeAndDist::SLDist>() = CurrDist;

			//UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] %s is moving further to %s; PrevDist=%f; CurrDist=%f; DiffDist=%f; [%s - %s];"),
			//	*FString(__func__), __LINE__, 
			//	CurrTimestamp, 
			//	*SemanticOwner.Obj->GetName(), 
			//	*CanidateData.Key->GetName(),
			//	PrevDist, CurrDist, DiffDist,
			//	*GetOwner()->GetActorLocation().ToString(),
			//	*CanidateData.Key->GetActorLocation().ToString());
		}
		else
		{
			// TODO reset time when idling for a longer period
			//UE_LOG(LogTemp, Log, TEXT("%s::%d [%f] %s is idling relative to %s; PrevDist=%f; CurrDist=%f; DiffDist=%f; [%s - %s];"),
			//	*FString(__func__), __LINE__, 
			//	CurrTimestamp, 
			//	*SemanticOwner.Obj->GetName(), 
			//	*CanidateData.Key->GetName(),
			//	PrevDist, CurrDist, DiffDist,
			//	*GetOwner()->GetActorLocation().ToString(),
			//	*CanidateData.Key->GetActorLocation().ToString());
		}
	}
}

// Publish currently overlapping components
void USLReachAndPreGraspMonitor::TriggerInitialOverlaps()
{
	// If objects are already overlapping at begin play, they will not be triggered
	// Here we do a manual overlap check and forward them to OnOverlapBegin
	TSet<UPrimitiveComponent*> CurrOverlappingComponents;
	GetOverlappingComponents(CurrOverlappingComponents);
	const FHitResult Dummy;
	for (const auto& CompItr : CurrOverlappingComponents)
	{
		OnOverlapBegin(this, CompItr->GetOwner(), CompItr, 0, false, Dummy);
	}
}

// Check if the object is can be a candidate for reaching
//bool USLReachAndPreGraspMonitor::CanBeACandidate(AStaticMeshActor* InObject) const
bool USLReachAndPreGraspMonitor::CanBeACandidate(AActor* InOther) const
{
	return InOther->IsA(AStaticMeshActor::StaticClass());

	//UActorComponent* AC = InObject->GetComponentByClass(USLIndividualComponent::StaticClass());
	//return AC != nullptr;
	
	//// Check if the object is movable
	//if (!InObject->IsRootComponentMovable())
	//{
	//	return false;
	//}
	
	//// Check if actor has a valid static mesh component
	//if (UStaticMeshComponent* SMC = InObject->GetStaticMeshComponent())
	//{
	//	// Commented out since handles can be grasped and have no physics on
	//	//// Check if component has physics on
	//	//if (!SMC->IsSimulatingPhysics())
	//	//{
	//	//	return false;
	//	//}

	//	// Check that object is not too heavy/large
	//	if (SMC->GetMass() < ObjectWeightLimit &&
	//		InObject->GetComponentsBoundingBox().GetVolume() < ObjectVolumeLimit)
	//	{
	//		return true;
	//	}
	//}

	//return false;
}

// Checks for candidates in the overlap area
void USLReachAndPreGraspMonitor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// Check if the component or its outer is semantically annotated
	USLBaseIndividual* OtherIndividual = FSLIndividualUtils::GetIndividualObject(OtherActor);
	if (OtherIndividual == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d %s is not annotated, this should not happen.."), *FString(__FUNCTION__), __LINE__, *OtherActor->GetName());
		return;
	}

	FString DebugLogString = FString::Printf(TEXT("%s::%d::%.4fs \t OverlapBegin: \t %s:%s;"),
		*FString(__FUNCTION__), __LINE__, GetWorld()->GetTimeSeconds(), *OtherActor->GetName(), *OtherComp->GetName());
	
	// Check if the individual can be a reach candidate
	if (CanBeACandidate(OtherActor))
	{
		const float Dist = FVector::Distance(GetOwner()->GetActorLocation(), OtherActor->GetActorLocation());
		CandidatesData.Emplace(OtherIndividual, MakeTuple(GetWorld()->GetTimeSeconds(), Dist));

		// Make sure the candidate update check is running
		if (!IsComponentTickEnabled())
		{
			SetComponentTickEnabled(true);
		}

		DebugLogString.Append("\t ADD as candidate;");
	}

	UE_LOG(LogTemp, Warning, TEXT("%s"), *DebugLogString);
}

// Checks for candidates in the overlap area
void USLReachAndPreGraspMonitor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	// Check if the component or its outer is semantically annotated
	USLBaseIndividual* OtherIndividual = FSLIndividualUtils::GetIndividualObject(OtherActor);
	if (OtherIndividual == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d %s is not annotated, this should not happen.."), *FString(__FUNCTION__), __LINE__, *OtherActor->GetName());
		return;
	}

	FString DebugLogString = FString::Printf(TEXT("%s::%d::%.4fs \t OverlapBegin: \t %s:%s;"),
		*FString(__FUNCTION__), __LINE__, GetWorld()->GetTimeSeconds(), *OtherActor->GetName(), *OtherComp->GetName());

	if (CandidatesData.Remove(OtherIndividual) > 0)
	{
		// Stop candidate update if this was the last candidate
		if (CandidatesData.Num() == 0)
		{
			SetComponentTickEnabled(false);
		}

		DebugLogString.Append("\t RM as candidate;");
	}

	UE_LOG(LogTemp, Error, TEXT("%s"), *DebugLogString);
}


// Called when sibling detects a grasp, used for ending the manipulator positioning event
void USLReachAndPreGraspMonitor::OnSLGraspBegin(USLBaseIndividual* Self, USLBaseIndividual* Other, float Timestamp, const FString& GraspType)
{
	if(CurrGraspedIndividual)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d::%.4fs Already grasping %s, cannot set a new grasp to %s.."), *FString(__FUNCTION__), __LINE__, GetWorld()->GetTimeSeconds(),
			*CurrGraspedIndividual->GetParentActor()->GetName(), *Other->GetParentActor()->GetName());
		return;
	}

	// Check if the grasped object is a candidate and is in contact with the hand
	if(FSLTimeAndDist* CandidateTimeAndDist = CandidatesData.Find(Other))
	{
		// TODO this could be an outdated time due to the delay, it however makes sense to keep it this way
		// since if there is a grasp with the object, it should also be in contact with
		if(float* ContactTime = ManipulatorContactData.Find(Other))
		{
			// Grasp is active, ignore future contact/grasp events
			CurrGraspedIndividual = Other;

			//UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f] %s set as grasped object.."),
			//	*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetName());

			// Cancel delay callback if active
			GetWorld()->GetTimerManager().ClearTimer(ManipulatorContactDelayTimerHandle);

			// Broadcast reach and pre grasp events
			const float ReachStartTime = CandidateTimeAndDist->Get<ESLTimeAndDist::SLTime>();
			const float ReachEndTime = *ContactTime;
			OnReachAndPreGraspEvent.Broadcast(OwnerIndividualObject, Other, ReachStartTime, ReachEndTime, Timestamp);

			// Remove existing candidates and pause the update callback while the hand is grasping
			CandidatesData.Empty();
			ManipulatorContactData.Empty();
			SetComponentTickEnabled(false);

			// Disable overlaps until grasp is released
			SetGenerateOverlapEvents(false);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] Grasped %s is not in the objects in contact with the manipulator list, this should not happen.."),
				*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetParentActor()->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] Grasped %s is not in the candidates list, this should not happen.."),
			*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetParentActor()->GetName());
	}

}

// Reset looking for the events
void USLReachAndPreGraspMonitor::OnSLGraspEnd(USLBaseIndividual* Self, USLBaseIndividual* Other, float Time)
{	
	if (CurrGraspedIndividual == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d::%.4fs No object is currently grasped while ending theoretical grasp with %s.."),
			*FString(__FUNCTION__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetParentActor()->GetName());
		return;
	}

	if(CurrGraspedIndividual == Other)
	{
		CurrGraspedIndividual = nullptr;
		//UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f] %s removed as grasped object.."),
		//	*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d::%.4fs End grasp with %s while %s is still grasped.. ignoring event.."),
			*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(),
			*Other->GetParentActor()->GetName(), *CurrGraspedIndividual->GetParentActor()->GetName());
		return;
	}

	// Start looking for new candidates
	TriggerInitialOverlaps();
	
	// Grasp released start listening to overlaps
	SetGenerateOverlapEvents(false);
}

// Called when the sibling is in contact with an object, used for ending the reaching event and starting the manipulator positioning event
void USLReachAndPreGraspMonitor::OnSLManipulatorContactBegin(const FSLContactResult& ContactResult)
{
	if(CurrGraspedIndividual)
	{
		// Ignore any manipulator contacts while in grasp mode
		return;
	}

	// Check if the object in contact with is one of the candidates (should be)
	if (CandidatesData.Contains(ContactResult.Other))
	{
		// Check if the contact should be concatenated 
		if(!SkipRecentManipulatorContactEndEventTime(ContactResult.Other, ContactResult.Time))
		{
			// Overwrite previous time or create a new contact result
			ManipulatorContactData.Emplace(ContactResult.Other, ContactResult.Time);
			//UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f] %s added as object in contact with the manipulator.."),
			//	*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *AsSMA->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] %s is in contact with the manipulator, but it is not in the candidates list, this should not happen.. "),
			*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *ContactResult.Other->GetParentActor()->GetName());
	}

}

// Manipulator is not in contact with object anymore, check for possible concatenation, or reset the potential reach time
void USLReachAndPreGraspMonitor::OnSLManipulatorContactEnd(USLBaseIndividual* Self, USLBaseIndividual* Other, float Time)
{
	if(CurrGraspedIndividual)
	{
		// Ignore any manipulator contacts while in grasp mode
		return;
	}

	// Check contact with manipulator (remove in delay callback, give concatenation a chance)
	if (CandidatesData.Contains(Other))
	{
		// Cache the event
		RecentlyEndedManipulatorContactEvents.Emplace(Other, Time);

		if (!GetWorld())
		{
			// The episode finished, going further is futile
			return;
		}

		// Delay reseting the reach time, it might be a small disconnection with the hand
		if(!GetWorld()->GetTimerManager().IsTimerActive(ManipulatorContactDelayTimerHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(ManipulatorContactDelayTimerHandle, this, &USLReachAndPreGraspMonitor::DelayedManipulatorContactEndEventCallback,
				MaxPreGraspEventTimeGap * 1.2f, false);
		}
	}
	else
	{
		// 
		// It can happen, during the grasp there is a contact with the manipulator
		// when the contact ends after the grasp, this gets called and there are no items in ObjectsInContactWithManipulator
		//UE_LOG(LogTemp, Error, TEXT("%s::%d This should not happen.."), *FString(__func__), __LINE__);
	}

}

// Delayed call of setting finished event to check for possible concatenation of jittering events of the same type
void USLReachAndPreGraspMonitor::DelayedManipulatorContactEndEventCallback()
{
	// Curr time (keep very recently added events for another delay)
	const float CurrTime = GetWorld()->GetTimeSeconds();
	
	for (auto EvItr(RecentlyEndedManipulatorContactEvents.CreateIterator()); EvItr; ++EvItr)
	{
		// If enough time has passed, reset the reach time
		if(CurrTime - EvItr->Time > MaxPreGraspEventTimeGap)
		{
			// Reset reach start in the candidate
			if(FSLTimeAndDist* TimeAndDist = CandidatesData.Find(EvItr->Other))
			{
				// No new contact happened, remove and reset reach time
				if(ManipulatorContactData.Remove(EvItr->Other) > 0)
				{
					//UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f] %s removed as object in contact with the manipulator.. (after delay, contact end time=%f)"),
					//	*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *EvItr->Other->GetName(), EvItr->Timestamp);
					TimeAndDist->Get<ESLTimeAndDist::SLTime>() = GetWorld()->GetTimeSeconds();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] %s is not in the contact list.. this should not happen.."),
						*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *EvItr->Other->GetName());
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] %s is not in the candidates list.. this should not happen.."),
					*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *EvItr->Other->GetName());
			}
			
			EvItr.RemoveCurrent();
		}
	}

	// There are very recent events still available, spin another delay callback to give them a chance to concatenate
	if(RecentlyEndedManipulatorContactEvents.Num() > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(ManipulatorContactDelayTimerHandle, this, &USLReachAndPreGraspMonitor::DelayedManipulatorContactEndEventCallback,
			MaxPreGraspEventTimeGap * 1.2f, false);
	}
}

// Check if this begin event happened right after the previous one ended, if so remove it from the array, and cancel publishing the begin event
bool USLReachAndPreGraspMonitor::SkipRecentManipulatorContactEndEventTime(USLBaseIndividual* Other, float StartTime)
{
	for (auto EvItr(RecentlyEndedManipulatorContactEvents.CreateIterator()); EvItr; ++EvItr)
	{
		// Check if it is an event between the same entities
		if(EvItr->Other == Other)
		{
			// Check time difference between the previous and current event
			const float TimeGap = StartTime - EvItr->Time;
			if(TimeGap < MaxPreGraspEventTimeGap)
			{
				// Event will be concatenated
				EvItr.RemoveCurrent();

				// Check if it was the last event, if so, pause the delay publisher
				if(RecentlyEndedManipulatorContactEvents.Num() == 0)
				{
					GetWorld()->GetTimerManager().ClearTimer(ManipulatorContactDelayTimerHandle);
				}
				return true;
			}
		}
	}
	return false;
}

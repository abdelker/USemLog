// Copyright 2018, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "SLSemanticMapWriter.h"
#include "PhysicsEngine/PhysicsConstraintActor.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Animation/SkeletalMeshActor.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

// UOwl
#include "OwlSemanticMapStatics.h"
//#include "Templates/OwlSemanticMapIAIKitchen.h"
//#include "Templates/OwlSemanticMapIAISupermarket.h"

// UUtils
#include "Tags.h"
#include "Ids.h"
#include "Conversions.h"

// Default constructor
FSLSemanticMapWriter::FSLSemanticMapWriter()
{
}

// Write semantic map to file
bool FSLSemanticMapWriter::WriteToFile(UWorld* World,
	EMapTemplate TemplateType,
	const FString& InDirectory,
	const FString& InFilename)
{
	// Create the semantic map template
	TSharedPtr<FOwlSemanticMap> SemMap = CreateSemanticMapTemplate(TemplateType);

	// Add entries to map
	AddAllEntries(SemMap, World);

	// Write map to file
	FString FullFilePath = FPaths::ProjectDir() +
		InDirectory + TEXT("/") + InFilename + TEXT(".owl");
	FPaths::RemoveDuplicateSlashes(FullFilePath);
	return FFileHelper::SaveStringToFile(SemMap->ToString(), *FullFilePath);
}

// Create semantic map template
TSharedPtr<FOwlSemanticMap> FSLSemanticMapWriter::CreateSemanticMapTemplate(EMapTemplate TemplateType)
{
	const FString MapId = FIds::NewGuidInBase64Url();
	if (TemplateType == EMapTemplate::Default)
	{
		return FOwlSemanticMapStatics::CreateDefaultSemanticMap(MapId);
	}
	else if (TemplateType == EMapTemplate::IAIKitchen)
	{
		return FOwlSemanticMapStatics::CreateIAIKitchenSemanticMap(MapId);
	}
	else if (TemplateType == EMapTemplate::IAISupermarket)
	{
		return FOwlSemanticMapStatics::CreateIAISupermarketSemanticMap(MapId);
	}
	return MakeShareable(new FOwlSemanticMap());
}

// Add entries to the semantic map
void FSLSemanticMapWriter::AddAllEntries(TSharedPtr<FOwlSemanticMap> InSemMap, UWorld* World)
{
	// Iterate objects with SemLog tag key
	for (const auto& ObjToTagsItr : FTags::GetObjectsToKeyValuePairs(World, "SemLog"))
	{
		// Cache some tag id results
		const FString Id = ObjToTagsItr.Value.Contains("Id") ?
			ObjToTagsItr.Value["Id"] : TEXT("");
		const FString Class = ObjToTagsItr.Value.Contains("Class") ?
			ObjToTagsItr.Value["Class"] : TEXT("");

		// Take into account only objects with an id
		if (!Id.IsEmpty())
		{
			// Check if 
			if (!Class.IsEmpty())
			{				
				AddObjectEntry(InSemMap, ObjToTagsItr.Key, Id, Class);
			}
			else if (APhysicsConstraintActor* ConstrAct = Cast<APhysicsConstraintActor>(ObjToTagsItr.Key))
			{
				AddConstraintEntry(InSemMap, ConstrAct->GetConstraintComp(), Id);
			}
			else if (UPhysicsConstraintComponent* ConstrComp = Cast<UPhysicsConstraintComponent>(ObjToTagsItr.Key))
			{
				AddConstraintEntry(InSemMap, ConstrComp, Id);
			}
		}

		// Add class entries (Id not mandatory)
		if (!Class.IsEmpty())
		{
			const FString ParentClass = ObjToTagsItr.Value.Contains("ParentClass") ?
				ObjToTagsItr.Value["ParentClass"] : TEXT("");
			AddClassDefinition(InSemMap, ObjToTagsItr.Key, Class, ParentClass);
		}
	}
}

// Add object entry to the semantic map
void FSLSemanticMapWriter::AddObjectEntry(TSharedPtr<FOwlSemanticMap> InSemMap,
	UObject* Object,
	const FString& InId,
	const FString& InClass)
{
	const FString MapPrefix = InSemMap->MapPrefix;
	// Create the object individual
	FOwlNode ObjIndividual = FOwlStatics::CreateObjectIndividual(
		MapPrefix, InId, InClass);
	
	// Add parent property
	const FString ParentId = GetParentId(Object);
	if (!ParentId.IsEmpty())
	{
		ObjIndividual.AddChildNode(FOwlStatics::CreateParentProperty(
			MapPrefix, ParentId));
	}

	// Add children properties
	for (const auto& ChildId : GetAllChildIds(Object))
	{
		ObjIndividual.AddChildNode(FOwlStatics::CreateChildProperty(
			MapPrefix, ChildId));
	}

	// Add pose individual to map
	if (AActor* ActEntry = Cast<AActor>(Object))
	{
		// Generate unique id for the pose individual
		const FString PoseId = FIds::NewGuidInBase64Url();

		// Add properties
		ObjIndividual.AddChildNode(FOwlStatics::CreatePoseProperty(
			MapPrefix, PoseId));

		// Add entries 
		InSemMap->AddEntry(ObjIndividual);

		// Create pose entry
		const FVector ROSLoc = FConversions::UToROS(ActEntry->GetActorLocation());
		const FQuat ROSQuat = FConversions::UToROS(ActEntry->GetActorQuat());
		InSemMap->AddEntry(FOwlStatics::CreatePoseIndividual(
			MapPrefix, PoseId, ROSLoc, ROSQuat));
	}
	else if (USceneComponent* CompEntry = Cast<USceneComponent>(Object))
	{
		// Generate unique id for the pose individual
		const FString PoseId = FIds::NewGuidInBase64Url();

		// Add properties
		ObjIndividual.AddChildNode(FOwlStatics::CreatePoseProperty(
			MapPrefix, PoseId));
		
		// Add entries 
		InSemMap->AddEntry(ObjIndividual);

		// Create pose entry
		const FVector ROSLoc = FConversions::UToROS(CompEntry->GetComponentLocation());
		const FQuat ROSQuat = FConversions::UToROS(CompEntry->GetComponentQuat());
		InSemMap->AddEntry(FOwlStatics::CreatePoseIndividual(
			MapPrefix, PoseId, ROSLoc, ROSQuat));
	}
	else
	{
		// Obj has no pose info
		InSemMap->AddEntry(ObjIndividual);
	}
}

// Add class entry
void FSLSemanticMapWriter::AddClassDefinition(TSharedPtr<FOwlSemanticMap> InSemMap,
	UObject* Object,
	const FString& InClass,
	const FString& InParentClass)
{
	// Return if class was already defined
	for (const auto& ClassDef : InSemMap->ClassDefinitions)
	{
		for (const auto& ClassAttr : ClassDef.Attributes)
		{
			if (ClassAttr.Key.Prefix.Equals("rdf") &&
				ClassAttr.Key.LocalName.Equals("about") &&
				ClassAttr.Value.LocalValue.Equals(InClass))
			{
				return;
			}
		}
	}

	// Create class definition entry
	FOwlNode ClassDefinition = FOwlStatics::CreateClassDefinition(InClass);
	ClassDefinition.Comment = TEXT("Class ") + InClass;

	// Check if subclass is known
	if (!InParentClass.IsEmpty())
	{
		ClassDefinition.AddChildNode(FOwlStatics::CreateSubClassOfProperty(InParentClass));
	}

	// Add bounds if available
	if (AStaticMeshActor* ObjAsSMAct = Cast<AStaticMeshActor>(Object))
	{
		if (UStaticMeshComponent* SMComp = ObjAsSMAct->GetStaticMeshComponent())
		{
			const FVector BBSize = FConversions::CmToM(
				SMComp->Bounds.GetBox().GetSize());
			if (!BBSize.IsZero())
			{
				ClassDefinition.AddChildNode(FOwlStatics::CreateDepthProperty(BBSize.X));
				ClassDefinition.AddChildNode(FOwlStatics::CreateWidthProperty(BBSize.Y));
				ClassDefinition.AddChildNode(FOwlStatics::CreateHeightProperty(BBSize.Z));
			}
		}
	}
	else if (ASkeletalMeshActor* ObjAsSkelAct = Cast<ASkeletalMeshActor>(Object))
	{
		if (USkeletalMeshComponent* SkelComp = ObjAsSkelAct->GetSkeletalMeshComponent())
		{
			const FVector BBSize = FConversions::CmToM(
				SkelComp->Bounds.GetBox().GetSize());
			if (!BBSize.IsZero())
			{
				ClassDefinition.AddChildNode(FOwlStatics::CreateDepthProperty(BBSize.X));
				ClassDefinition.AddChildNode(FOwlStatics::CreateWidthProperty(BBSize.Y));
				ClassDefinition.AddChildNode(FOwlStatics::CreateHeightProperty(BBSize.Z));
			}

			TArray<FName> BoneNames;
			SkelComp->GetBoneNames(BoneNames);
			for (const auto& BoneName : BoneNames)
			{
				ClassDefinition.AddChildNode(
					FOwlStatics::CreateSkeletalBoneProperty(BoneName.ToString()));
			}
		}
	}
	else if (USkeletalMeshComponent* ObjAsSkelComp = Cast<USkeletalMeshComponent>(Object))
	{
		const FVector BBSize = FConversions::CmToM(
			ObjAsSkelComp->Bounds.GetBox().GetSize());
		if (!BBSize.IsZero())
		{
			ClassDefinition.AddChildNode(FOwlStatics::CreateDepthProperty(BBSize.X));
			ClassDefinition.AddChildNode(FOwlStatics::CreateWidthProperty(BBSize.Y));
			ClassDefinition.AddChildNode(FOwlStatics::CreateHeightProperty(BBSize.Z));
		}

		TArray<FName> BoneNames;
		ObjAsSkelComp->GetBoneNames(BoneNames);
		for (const auto& BoneName : BoneNames)
		{
			ClassDefinition.AddChildNode(
				FOwlStatics::CreateSkeletalBoneProperty(BoneName.ToString()));
		}
	}
	else if(UPrimitiveComponent* ObjAsPrimComp = Cast<UPrimitiveComponent>(Object))
	{
		const FVector BBSize = FConversions::CmToM(
			ObjAsPrimComp->Bounds.GetBox().GetSize());
		if (!BBSize.IsZero())
		{
			ClassDefinition.AddChildNode(FOwlStatics::CreateDepthProperty(BBSize.X));
			ClassDefinition.AddChildNode(FOwlStatics::CreateWidthProperty(BBSize.Y));
			ClassDefinition.AddChildNode(FOwlStatics::CreateHeightProperty(BBSize.Z));
		}
	}
	InSemMap->ClassDefinitions.Add(ClassDefinition);
}

// Add constraint entry
void FSLSemanticMapWriter::AddConstraintEntry(TSharedPtr<FOwlSemanticMap> InSemMap,
	UPhysicsConstraintComponent* ConstraintComp,
	const FString& InId)
{
	const FString MapPrefix = InSemMap->MapPrefix;
	AActor* ParentAct = ConstraintComp->ConstraintActor1;
	AActor* ChildAct = ConstraintComp->ConstraintActor2;
	if (ParentAct && ChildAct)
	{
		const FString ParentId = FTags::GetKeyValue(ParentAct, "SemLog", "Id");
		const FString ChildId = FTags::GetKeyValue(ChildAct, "SemLog", "Id");
		if (!ParentId.IsEmpty() && !ChildId.IsEmpty())
		{
			// Create the object individual
			FOwlNode ConstrIndividual = FOwlStatics::CreateConstraintIndividual(
				MapPrefix, InId, ParentId, ChildId);

			// Generate unique ids 
			const FString PoseId = FIds::NewGuidInBase64Url();
			const FString LinId = FIds::NewGuidInBase64Url();
			const FString AngId = FIds::NewGuidInBase64Url();

			// Add properties
			ConstrIndividual.AddChildNode(FOwlStatics::CreatePoseProperty(
				MapPrefix, PoseId));
			ConstrIndividual.AddChildNode(FOwlStatics::CreateLinearConstraintProperty(
				MapPrefix, LinId));
			ConstrIndividual.AddChildNode(FOwlStatics::CreateAngularConstraintProperty(
				MapPrefix, AngId));
			
			// Add entries to the map
			InSemMap->AddEntry(ConstrIndividual);

			// Create pose entry
			const FVector ROSLoc = FConversions::UToROS(ConstraintComp->GetComponentLocation());
			const FQuat ROSQuat = FConversions::UToROS(ConstraintComp->GetComponentQuat());
			InSemMap->AddEntry(FOwlStatics::CreatePoseIndividual(
				MapPrefix, PoseId, ROSLoc, ROSQuat));

			// Create linear constraint entry
			const uint8 LinXMotion = ConstraintComp->ConstraintInstance.GetLinearXMotion();
			const uint8 LinYMotion = ConstraintComp->ConstraintInstance.GetLinearYMotion();
			const uint8 LinZMotion = ConstraintComp->ConstraintInstance.GetLinearZMotion();
			const float LinLimit = FConversions::CmToM(ConstraintComp->ConstraintInstance.GetLinearLimit());
			const bool bLinSoftConstraint = false; // ConstraintComp->ConstraintInstance.ProfileInstance.
			const float LinStiffness = 0.f; // ConstraintComp->ConstraintInstance.ProfileInstance.
			const float LinDamping = 0.f; // ConstraintComp->ConstraintInstance.ProfileInstance.

			InSemMap->AddEntry(FOwlStatics::CreateLinearConstraintProperties(
				MapPrefix, LinId, LinXMotion, LinYMotion, LinZMotion, LinLimit, 
				bLinSoftConstraint, LinStiffness, LinDamping));

			// Create angular constraint entry
			const uint8 AngSwing1Motion = ConstraintComp->ConstraintInstance.GetAngularSwing1Motion();
			const uint8 AngSwing2Motion = ConstraintComp->ConstraintInstance.GetAngularSwing2Motion();
			const uint8 AngTwistMotion = ConstraintComp->ConstraintInstance.GetAngularTwistMotion();
			const float AngSwing1Limit = FMath::DegreesToRadians(ConstraintComp->ConstraintInstance.GetAngularSwing1Limit());
			const float AngSwing2Limit = FMath::DegreesToRadians(ConstraintComp->ConstraintInstance.GetAngularSwing2Limit());
			const float AngTwistLimit = FMath::DegreesToRadians(ConstraintComp->ConstraintInstance.GetAngularTwistLimit());
			const bool bAngSoftSwingConstraint = false;
			const float AngSwingStiffness = 0.f;
			const float AngSwingDamping = 0.f;
			const bool bAngSoftTwistConstraint = false;
			const float AngTwistStiffness = 0.f;
			const float AngTwistDamping = 0.f;

			InSemMap->AddEntry(FOwlStatics::CreateAngularConstraintProperties(
				MapPrefix, AngId, AngSwing1Motion, AngSwing2Motion, AngTwistMotion,
				AngSwing1Limit, AngSwing2Limit, AngTwistLimit, bAngSoftSwingConstraint,
				AngSwingStiffness, AngSwingDamping, bAngSoftTwistConstraint,
				AngTwistStiffness, AngTwistDamping));
		}
	}
}

// Get children ids (only direct children, no grandchildren etc.)
TArray<FString> FSLSemanticMapWriter::GetAllChildIds(UObject* Object)
{
	// Array of children ids
	TArray<FString> Ids;

	// Check object type
	if (AActor* ObjAsActor = Cast<AActor>(Object))
	{
		// Iterate child actors (only direct children, no grandchildren etc.)
		TArray<AActor*> ChildActors;
		ObjAsActor->GetAllChildActors(ChildActors, false);
		for (const auto& ChildAct : ChildActors)
		{
			const FString ChildId = FTags::GetKeyValue(ChildAct, "SemLog", "Id");
			if (!ChildId.IsEmpty() && FTags::HasKey(ChildAct, "SemLog", "Class"))
			{
				Ids.Add(ChildId);
			}
		}

		// Iterate child components (only direct children, no grandchildren etc.)
		TInlineComponentArray<UActorComponent*> ChildComponents;
		ObjAsActor->GetComponents(ChildComponents, false);
		for (const auto& ChildComp : ChildComponents)
		{
			const FString ChildId = FTags::GetKeyValue(ChildComp, "SemLog", "Id");
			if (!ChildId.IsEmpty() && FTags::HasKey(ChildComp, "SemLog", "Class"))
			{
				Ids.Add(ChildId);
			}
		}
	}
	// Only scene components can have other components as children
	else if (USceneComponent* ObjAsSceneComp = Cast<USceneComponent>(Object))
	{
		// Iterate child components (only direct children, no grandchildren etc.)
		TArray<USceneComponent*> ChildComponents;
		ObjAsSceneComp->GetChildrenComponents(false, ChildComponents);
		for (const auto& ChildComp : ChildComponents)
		{
			const FString ChildId = FTags::GetKeyValue(ChildComp, "SemLog", "Id");
			if (!ChildId.IsEmpty() && FTags::HasKey(ChildComp, "SemLog", "Class"))
			{
				Ids.Add(ChildId);
			}
		}
	}
	return Ids;
}

// Get parent id (empty string if none)
FString FSLSemanticMapWriter::GetParentId(UObject* Object)
{
	// Check object type
	if (AActor* ObjAsAct = Cast<AActor>(Object))
	{
		AActor* ParentAct = ObjAsAct->GetParentActor();
		const FString ParentActId = FTags::GetKeyValue(ParentAct, "SemLog", "Id");
		if (!ParentActId.IsEmpty() && FTags::HasKey(ParentAct, "SemLog", "Class"))
		{
			return ParentActId;
		}

		// If this actor was created by a child actor component
		UChildActorComponent* ParentComp = ObjAsAct->GetParentComponent();
		const FString ParentCompId = FTags::GetKeyValue(ParentComp, "SemLog", "Id");
		if (!ParentCompId.IsEmpty() && FTags::HasKey(ParentComp, "SemLog", "Class"))
		{
			return ParentCompId;
		}
	}
	else if (USceneComponent* ObjAsSceneComp = Cast<USceneComponent>(Object))
	{
		USceneComponent* AttachComp = ObjAsSceneComp->GetAttachParent();
		const FString AttachCompId = FTags::GetKeyValue(AttachComp, "SemLog", "Id");
		if (!AttachCompId.IsEmpty() && FTags::HasKey(AttachComp, "SemLog", "Class"))
		{
			return AttachCompId;
		}

		AActor* ParentAct = ObjAsSceneComp->GetAttachmentRootActor();
		const FString ParentActId = FTags::GetKeyValue(ParentAct, "SemLog", "Id");
		if (!ParentActId.IsEmpty() && FTags::HasKey(ParentAct, "SemLog", "Class"))
		{
			return ParentActId;
		}
	}
	else if (UActorComponent* ObjAsActComp = Cast<USceneComponent>(Object))
	{
		AActor* Owner = ObjAsActComp->GetOwner();
		const FString OwnerId = FTags::GetKeyValue(Owner, "SemLog", "Id");
		if (!OwnerId.IsEmpty() && FTags::HasKey(Owner, "SemLog", "Class"))
		{
			return OwnerId;
		}
	}
	// No parent
	return FString();
}

// Copyright 2017-2020, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SLIndividualBase.h"
#include "Data/SLIndividual.h"
#include "Data/SLIndividualUtils.h"
#include "SLIndividualComponent.generated.h"


UCLASS( ClassGroup=(SL), meta=(BlueprintSpawnableComponent), DisplayName = "SL Individual Component")
class USEMLOG_API USLIndividualComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USLIndividualComponent();

	// Called before destroying the object.
	virtual void BeginDestroy() override;

	// Called after the C++ constructor and after the properties have been initialized, including those loaded from config.
	virtual void PostInitProperties() override;

#if WITH_EDITOR
	// Called when a property is changed in the editor
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	// Called when a component is created (not loaded) (after post init).This can happen in the editor or during gameplay
	virtual void OnComponentCreated() override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Get the semantic individual object
	USLIndividualBase* GetIndividualObject() const { return SemanticIndividual; };

	// Get the semantic individual using a cast class (nullptr if cast is unsuccessfull)
	template <typename ClassType>
	ClassType* GetCastedIndividualObject() const {	return Cast<ClassType>(SemanticIndividual); };

	// Save data to owners tag
	void SaveToTag(bool bOverwrite = false);

	// Load data from owners tag
	void LoadFromTag(bool bOverwrite = false);

	// Reload the individual data
	bool RefreshIndividual();

private:
	// Semantic data
	UPROPERTY(VisibleAnywhere, Category = "Semantic Logger")
	USLIndividualBase* SemanticIndividual;

	// Manually convert the semantic individual to the chosen type
	UPROPERTY(EditAnywhere, Category = "Semantic Logger|Manual Edit")
	TSubclassOf<class USLIndividual> ConvertTo;
	
	/* Button workarounds */
	// Ovewrite any changes
	UPROPERTY(EditAnywhere, Category = "Semantic Logger|Manual Edit")
	bool bOverwriteEditChanges;

	// Save data to tag
	UPROPERTY(EditAnywhere, Category = "Semantic Logger|Manual Edit")
	bool bSaveToTagButton;

	// Load data from tag
	UPROPERTY(EditAnywhere, Category = "Semantic Logger|Manual Edit")
	bool bLoadFromTagButton;

	// Switch between viewing the real and the visual mask color
	UPROPERTY(EditAnywhere, Category = "Semantic Logger|Manual Edit")
	bool bToggleVisualMaskMaterial;
};

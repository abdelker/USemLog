// Copyright 2017-2020, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#if SL_WITH_LIBMONGO_C
THIRD_PARTY_INCLUDES_START
	#if PLATFORM_WINDOWS
	#include "Windows/AllowWindowsPlatformTypes.h"
	#include <mongoc/mongoc.h>
	#include "Windows/HideWindowsPlatformTypes.h"
	#else
	#include <mongoc/mongoc.h>
	#endif // #if PLATFORM_WINDOWS
THIRD_PARTY_INCLUDES_END
#endif //SL_WITH_LIBMONGO_C
#include "SLBaseIndividual.generated.h"

// Notify every time the init status changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSLIndividualInitChangeSignature, USLBaseIndividual*, Individual, bool, bNewInitVal);

// Notify every time the loaded status changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSLIndividualLoadedChangeSignature, USLBaseIndividual*, Individual, bool, bNewLoadedVal);

// Notify every time the id changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSLIndividualNewIdSignature, USLBaseIndividual*, Individual, const FString&, NewId);

// Notify every time the class changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSLIndividualNewClassSignature, USLBaseIndividual*, Individual, const FString&, NewClass);

/**
 * 
 */
UCLASS(ClassGroup = SL)
class USLBaseIndividual : public UObject
{
	GENERATED_BODY()

public:
	// Ctor
	USLBaseIndividual();

	// Called before destroying the object.
	virtual void BeginDestroy() override;

	// Called after the C++ constructor and after the properties have been initialized, including those loaded from config.
	virtual void PostInitProperties() override;

	// Init asset references (bReset forces re-initialization)
	virtual bool Init(bool bReset = false);

	// Load semantic data (bReset forces re-loading)
	virtual bool Load(bool bReset = false, bool bTryImport = false);

	// Depenencies set
	bool IsInit() const { return bIsInit; };

	// Data loaded
	bool IsLoaded() const { return bIsLoaded; };

	// Save values externally
	virtual bool ExportValues(bool bOverwrite = false);

	// Load values externally
	virtual bool ImportValues(bool bOverwrite = false);

	// Clear exported values
	virtual bool ClearExportedValues();

	// Get actor represented by the individual
	AActor* GetParentActor() const { return ParentActor; };

	// True if individual is part of another individual
	bool IsAttachedToAnotherIndividual() const;

	// Return the actor this individual is part of
	class AActor* GetAttachedToActor() const { return AttachedToActor; };

	// Return the individual this individual is part of
	class USLBaseIndividual* GetAttachedToIndividual() const { return AttachedToIndividual; };

	// Get the type name as string
	virtual FString GetTypeName() const { return FString("BaseIndividual"); };

	/* Id */
	// Set the id value, if empty, reset the individual as not loaded
	void SetIdValue(const FString& NewVal);
	void ClearIdValue() { SetIdValue(""); };
	FString GetIdValue() const { return Id; };
	bool IsIdValueSet() const { return !Id.IsEmpty(); };

	/* Class */
	// Set the class value, if empty, reset the individual as not loaded
	void SetClassValue(const FString& NewClass);
	void ClearClassValue() { SetClassValue(""); };
	FString GetClassValue() const { return Class; };
	bool IsClassValueSet() const { return !Class.IsEmpty(); };

	/*OId*/
	// BSON Id of the individual, used for db optimized queries
	void SetOIdValue(const FString& NewVal);
	void ClearOIdValue() { SetOIdValue(""); };
	FString GetOIdValue() const { return OId; };
	bool IsOIdValueSet() const { return !OId.IsEmpty(); };
#if SL_WITH_LIBMONGO_C
	bson_oid_t GetOId() const { return oid; };
#endif // SL_WITH_LIBMONGO_C

protected:
	// Generate class name, virtual since each invidiual type will have different name
	virtual FString GetClassName() const;

	// Clear all references of the individual
	virtual void InitReset();

	// Clear all data of the individual
	virtual void LoadReset();

	// Clear any bound delegates (called when init is reset)
	virtual void ClearDelegateBounds();

	// Mark individual as init, broadcast change
	void SetIsInit(bool bNewValue, bool bBroadcast = true);

	// Mark individual as loaded, broadcast change
	void SetIsLoaded(bool bNewValue, bool bBroadcast = true);
	
	// Check that the parent actor is set and valid
	bool HasValidParentActor() const;

	// Set pointer to parent actor
	virtual bool SetParentActor();

	// Set attachment parent (part of individual)
	bool SetAttachedToParent();

private:
	// Set dependencies
	bool InitImpl();

	// Set data
	bool LoadImpl(bool bTryImport = true);

	// Generate a new id
	FString GenerateNewId() const;

	// Import values expernally
	bool ImportIdValue(bool bOverwrite = false);
	bool ImportClassValue(bool bOverwrite = false);
	bool ImportOIdValue(bool bOverwrite = false);

	// Load the oid value from the persitent OId string (generate a new one if none is available)
	bool LoadOId(bool bGenerateNew = false);

public:
	// Public delegates
	FSLIndividualInitChangeSignature OnInitChanged;
	FSLIndividualInitChangeSignature OnLoadedChanged;
	FSLIndividualNewIdSignature OnNewIdValue;
	FSLIndividualNewClassSignature OnNewClassValue;

protected:
	// Pointer to the actor described by the semantic description class
	UPROPERTY(VisibleAnywhere, Category = "SL")
	AActor* ParentActor;

	// Individual unique id
	UPROPERTY(VisibleAnywhere, Category = "SL")
	FString Id;

	// Idividual class
	UPROPERTY(VisibleAnywhere, Category = "SL")
	FString Class;

	// BSON Id of the individual as persistent string
	UPROPERTY(VisibleAnywhere, Category = "SL")
	FString OId;
	
	// True if individual is initialized
	UPROPERTY(VisibleAnywhere, Category = "SL")
	uint8 bIsInit : 1;

	// True if individual is loaded
	UPROPERTY(VisibleAnywhere, Category = "SL")
	uint8 bIsLoaded : 1;

	// Actor attached to
	UPROPERTY(VisibleAnywhere, Category = "SL")
	AActor* AttachedToActor;

	// Individual of the attached to actor
	UPROPERTY(VisibleAnywhere, Category = "SL")
	USLBaseIndividual* AttachedToIndividual;

#if SL_WITH_LIBMONGO_C
	// Native, non-persistent bson oid of the individual
	bson_oid_t oid;
#endif

	// SemLog tag key
	UPROPERTY(VisibleAnywhere, Category = "SL")
	FString ImportTagType;

	///* Constants */
	//// Tag type for exporting/importing data from tags
	//static constexpr char TagTypeConst[] = "SemLog";
};
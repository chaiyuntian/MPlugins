//Easy Multi Save - Copyright (C) 2020 by Michael Hegemann.  

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "EMSData.h"
#include "EMSActorSaveInterface.generated.h"

UINTERFACE(Category = "Easy Multi Save", BlueprintType, meta = (DisplayName = "EMS Save Interface"))
class UEMSActorSaveInterface : public UInterface
{
	GENERATED_BODY()
};

class EASYMULTISAVE_API IEMSActorSaveInterface
{
	GENERATED_BODY()

public:
	
	/**Executed after the Actor and all of it's components have been loaded.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Easy Multi Save")
	void ActorLoaded();

	/**Executed when the Actor and all of it's components have been saved.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Easy Multi Save")
	void ActorSaved();

	/**Executed before the Actor and all of it's components are saved.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Easy Multi Save")
	void ActorPreSave();

	/**
	* (Not Level Blueprints) Holds the array of components that you want to save for the Actor.
	* This function is not relevant for Level Blueprints, as they cannot have components.
	*
	* @param Components - The Components that you want to save with the Actor.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Easy Multi Save")
	void ComponentsToSave(TArray<UActorComponent*>& Components);

};


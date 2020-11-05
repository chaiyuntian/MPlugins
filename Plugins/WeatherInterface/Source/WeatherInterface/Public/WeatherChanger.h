// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeatherEvent.h"
#include "WeatherChanger.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UWeatherChanger : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class WEATHERINTERFACE_API IWeatherChanger
{
	GENERATED_BODY()
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(Blueprintcallable, BlueprintNativeEvent, Category="Weather Changer" )
	bool OnScalarValueChange(FName ParameterName, float Value);

	UFUNCTION(Blueprintcallable, BlueprintNativeEvent, Category = "Weather Changer")
	bool OnVectorValueChange(FName ParameterName, FVector Value);

	UFUNCTION(Blueprintcallable, BlueprintNativeEvent, Category = "Weather Changer")
	bool OnIntValueChange(FName ParameterName, int32 Value);

	UFUNCTION(Blueprintcallable, BlueprintNativeEvent, Category = "Weather Changer")
	bool OnWeatherPhenomenomChange(EWEATHER_PHENOMENOM WeatherPhenomenom);

};

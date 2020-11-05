// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeatherManagerBase.generated.h"

UCLASS()
class WEATHERINTERFACE_API AWeatherManagerBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeatherManagerBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(Blueprintpure, Category = "WeatherManager")
	TArray<AActor*> GetWeatherChangers(bool Latest = false);

	UFUNCTION(BlueprintCallable, Category = "WeatherManager")
	void UpdateWeatherChangers();

public:
	UPROPERTY(VisibleAnywhere,BlueprintReadonly,Category = "WeatherManager")
	TArray<AActor*> WeatherChangers;

};

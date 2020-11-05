// Fill out your copyright notice in the Description page of Project Settings.


#include "WeatherManagerBase.h"
#include "WeatherChanger.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AWeatherManagerBase::AWeatherManagerBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AWeatherManagerBase::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AWeatherManagerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeatherManagerBase::OnConstruction(const FTransform& Transform)
{
	UpdateWeatherChangers();
	Super::OnConstruction(Transform);
}

TArray<AActor*> AWeatherManagerBase::GetWeatherChangers(bool Latest /*= false*/)
{
	if (Latest)
	{
		UGameplayStatics::GetAllActorsWithInterface(this->GetWorld(), UWeatherChanger::StaticClass(), WeatherChangers);
	}
	return WeatherChangers;
}

void AWeatherManagerBase::UpdateWeatherChangers()
{
	UGameplayStatics::GetAllActorsWithInterface(this->GetWorld(), UWeatherChanger::StaticClass(), WeatherChangers);
}


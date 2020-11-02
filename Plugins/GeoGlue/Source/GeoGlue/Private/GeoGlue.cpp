// Copyright Epic Games, Inc. All Rights Reserved.

#include "GeoGlue.h"
#include "UnrealGDAL.h"

#define LOCTEXT_NAMESPACE "FGeoGlueModule"

void FGeoGlueModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FUnrealGDALModule* UnrealGDAL = FModuleManager::Get().LoadModulePtr<FUnrealGDALModule>("UnrealGDAL");
	UnrealGDAL->InitGDAL();
}

void FGeoGlueModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGeoGlueModule, GeoGlue)
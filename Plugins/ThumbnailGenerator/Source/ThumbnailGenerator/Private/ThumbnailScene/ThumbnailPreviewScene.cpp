// Mans Isaksson 2020

#include "ThumbnailPreviewScene.h"
#include "ThumbnailGeneratorSettings.h"
#include "ThumbnailGeneratorInterfaces.h"

#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/TextureCube.h"

FThumbnailPreviewScene::FThumbnailPreviewScene()
	: FPreviewScene(FPreviewScene::ConstructionValues()
					.SetCreateDefaultLighting(true)
					.SetLightRotation(FRotator(45.f, 0, 0))
					.SetSkyBrightness(2.f)
					.SetLightBrightness(4.f)

					.AllowAudioPlayback(false)
					.SetForceMipsResident(false)
					.SetCreatePhysicsScene(true)
					.ShouldSimulatePhysics(false)
					.SetTransactional(true)
					.SetEditor(false))
{
	// Disable ticking by the engine since we manage ticking on our own
	GetWorld()->SetShouldTick(false);

	const FThumbnailSettings& DefaultSettings = UThumbnailGeneratorSettings::Get()->DefaultThumbnailSettings;

	// Always set up sky light using the set cube map texture, reusing the sky light from PreviewScene class
	SetSkyCubemap(DefaultSettings.EnvironmentCubeMap.LoadSynchronous());
	SetSkyBrightness(DefaultSettings.SkyLightIntensity);
	SetLightDirection(DefaultSettings.DirectionalLightRotation);

	// Add Fill Light
	DirectionalFillLight = NewObject<UDirectionalLightComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	AddComponent(DirectionalFillLight, FTransform(DefaultSettings.DirectionalFillLightRotation));
	DirectionalFillLight->SetAbsolute(true, true, true);
	DirectionalFillLight->SetRelativeRotation(DefaultSettings.DirectionalFillLightRotation);
	DirectionalFillLight->SetLightColor(DefaultSettings.DirectionalFillLightColor);
	DirectionalFillLight->Intensity = DefaultSettings.DirectionalFillLightIntensity;

	UpdateScene(DefaultSettings, true);
}

bool FThumbnailPreviewScene::UpdateLightSources(const FThumbnailSettings& ThumbnailSettings, class UDirectionalLightComponent* DirectionalLight,
	class UDirectionalLightComponent* DirectionalFillLight, USkyLightComponent* SkyLight, bool bForceUpdate)
{
	bool bSkyLightChanged = false;

	// Update SkyLight
	if (SkyLight)
	{
		if (bForceUpdate || !FMath::IsNearlyEqual(SkyLight->Intensity, ThumbnailSettings.SkyLightIntensity, 0.05f))
		{
			SkyLight->SetIntensity(ThumbnailSettings.SkyLightIntensity);
			bSkyLightChanged = true;
		}

		if (bForceUpdate || !SkyLight->GetLightColor().Equals(ThumbnailSettings.SkyLightColor))
		{
			SkyLight->SetLightColor(ThumbnailSettings.SkyLightColor);
			bSkyLightChanged = true;
		}
	}

	// Update Environment
	if (DirectionalLight && SkyLight)
	{
		const FRotator LightDir = DirectionalLight->GetComponentTransform().GetUnitAxis(EAxis::X).Rotation();
		if (bForceUpdate || !FMath::IsNearlyEqual(LightDir.Yaw, ThumbnailSettings.EnvironmentRotation, 0.05f))
		{
			SkyLight->SourceCubemapAngle = ThumbnailSettings.EnvironmentRotation;
			bSkyLightChanged = true;
		}

		const auto DesiredSourceType = !ThumbnailSettings.bShowEnvironment || !ThumbnailSettings.bEnvironmentAffectLighting 
			? ESkyLightSourceType::SLS_SpecifiedCubemap 
			: ESkyLightSourceType::SLS_CapturedScene;

		if (bForceUpdate || SkyLight->SourceType != DesiredSourceType)
		{
			SkyLight->SourceType = DesiredSourceType;
			bSkyLightChanged = true;
		}

		UTextureCube* EnvironmentTexture = ThumbnailSettings.bEnvironmentAffectLighting
			? ThumbnailSettings.EnvironmentCubeMap.LoadSynchronous()
			: Cast<UTextureCube>(FSoftObjectPath("TextureCube'/ThumbnailGenerator/SkySphere/T_Thumbnail_CubeMap.T_Thumbnail_CubeMap'").TryLoad());
			
		if (bForceUpdate || SkyLight->Cubemap != EnvironmentTexture)
		{
			SkyLight->Cubemap = EnvironmentTexture;
			bSkyLightChanged = true;
		}
	}

	// Update Directional Light
	if (DirectionalLight)
	{
		if (bForceUpdate || !FMath::IsNearlyEqual(DirectionalLight->Intensity, ThumbnailSettings.DirectionalLightIntensity, 0.05f))
		{
			DirectionalLight->SetIntensity(ThumbnailSettings.DirectionalLightIntensity);
		}

		if (bForceUpdate || !DirectionalLight->GetLightColor().Equals(ThumbnailSettings.DirectionalLightColor))
		{
			DirectionalLight->SetLightColor(ThumbnailSettings.DirectionalLightColor);
		}

		if (bForceUpdate || !DirectionalLight->GetRelativeRotation().Equals(ThumbnailSettings.DirectionalLightRotation))
		{
			DirectionalLight->SetRelativeRotation(ThumbnailSettings.DirectionalLightRotation);
		}
	}

    // Update Directional Fill Light
	if (DirectionalFillLight)
    {
        if (bForceUpdate || !FMath::IsNearlyEqual(DirectionalFillLight->Intensity, ThumbnailSettings.DirectionalFillLightIntensity, 0.05f))
        {
            DirectionalFillLight->SetIntensity(ThumbnailSettings.DirectionalFillLightIntensity);
        }

        if (bForceUpdate || !DirectionalFillLight->GetLightColor().Equals(ThumbnailSettings.DirectionalFillLightColor))
        {
            DirectionalFillLight->SetLightColor(ThumbnailSettings.DirectionalFillLightColor);
        }

        if (bForceUpdate || !DirectionalFillLight->GetRelativeRotation().Equals(ThumbnailSettings.DirectionalFillLightRotation))
        {
            DirectionalFillLight->SetRelativeRotation(ThumbnailSettings.DirectionalFillLightRotation);
        }
    }

	return bSkyLightChanged;
}

bool FThumbnailPreviewScene::UpdateSkySphere(const FThumbnailSettings& ThumbnailSettings, UWorld* World, AActor** SkySphereActorPtr, bool bForceUpdate)
{
	if (SkySphereActorPtr == nullptr)
		return false;

	AActor* SkySphereActor = SkySphereActorPtr ? *SkySphereActorPtr : nullptr;

	bool bSkyChanged = bForceUpdate;
	UClass* SkySphereClass = ThumbnailSettings.ThumbnailSkySphere.LoadSynchronous();
	if (IsValid(SkySphereActor) && (!SkySphereClass || !SkySphereActor->IsA(SkySphereClass)))
	{
		SkySphereActor->Destroy();
		SkySphereActor = nullptr;
		bSkyChanged = true;
	}

	if (!IsValid(SkySphereActor) && SkySphereClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.bNoFail = true;
		*SkySphereActorPtr = SkySphereActor = World->SpawnActor<AActor>(SkySphereClass, SpawnParams);
		bSkyChanged = true;
	}

	// Show/Hide sky Sphere
	if (IsValid(SkySphereActor))
	{
		if (ThumbnailSettings.bShowEnvironment && SkySphereActor->IsHidden())
		{
			SkySphereActor->SetActorHiddenInGame(false);
			bSkyChanged = true;
		}
		else if (!ThumbnailSettings.bShowEnvironment && !SkySphereActor->IsHidden())
		{
			SkySphereActor->SetActorHiddenInGame(true);
			bSkyChanged = true;
		}
	}

	if (bSkyChanged && IsValid(SkySphereActor) && SkySphereActor->Implements<UThumbnailSceneInterface>())
	{
		IThumbnailSceneInterface::Execute_OnUpdateThumbnailScene(SkySphereActor, ThumbnailSettings);
	}

	return bSkyChanged;
}

void FThumbnailPreviewScene::UpdateScene(const FThumbnailSettings& ThumbnailSettings, bool bForceUpdate)
{
	bool bSkyChanged = false;

	bSkyChanged |= UpdateLightSources(ThumbnailSettings, DirectionalLight, DirectionalFillLight, SkyLight, bForceUpdate);

	// Check if environment color has changed
	if (bForceUpdate || !LastEnvironmentColor.Equals(ThumbnailSettings.EnvironmentColor))
	{
		bSkyChanged = true;
		LastEnvironmentColor = ThumbnailSettings.EnvironmentColor;
	}

	bSkyChanged |= UpdateSkySphere(ThumbnailSettings, GetThumbnailWorld(), &SkySphereActor, bForceUpdate || bSkyChanged);

	if (bSkyChanged)
	{
		SkyLight->SetCaptureIsDirty();
		SkyLight->MarkRenderStateDirty();
		SkyLight->UpdateSkyCaptureContents(PreviewWorld);
	}
}

void FThumbnailPreviewScene::AddReferencedObjects(FReferenceCollector& Collector)
{
	FPreviewScene::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(SkySphereActor);
	Collector.AddReferencedObject(DirectionalFillLight);
}
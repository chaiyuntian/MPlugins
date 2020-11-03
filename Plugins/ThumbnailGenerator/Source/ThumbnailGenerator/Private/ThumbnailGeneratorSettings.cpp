// Mans Isaksson 2020

#include "ThumbnailGeneratorSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkinnedMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "ThumbnailGeneratorScript.h"
#include "ThumbnailGenerator.h"

FThumbnailSettings::FThumbnailSettings()
{
	// to set all bOverride_.. by default to false
	FMemory::Memzero(this, sizeof(FThumbnailSettings));

	ThumbnailTextureWidth = 512;
	ThumbnailTextureHeight = 512;

	bCaptureAlpha = false;
	AlphaBlendMode = EThumbnailAlphaBlendMode::EReplace;

	ProjectionType = ECameraProjectionMode::Perspective;
	CameraFitMode = EThumbnailCameraFitMode::EFit;
	CameraFOV = 45.f;
	OrthoWidthOffset = 0.f;
	CameraPivot = 18.f;
	CameraRotation = -22.f;
	CameraDistanceOffset = 0.f;
	CameraPositionOffset = FVector::ZeroVector;
	CameraRotationOffset = FRotator::ZeroRotator;
	
	SimulationMode = EThumbnailSceneSimulationMode::ESpecifiedComponents;
	SimulateSceneTime = 0.01f;
	SimulateSceneFramerate = 15.f;
	ComponentsToSimulate = { USkinnedMeshComponent::StaticClass(), UParticleSystemComponent::StaticClass() };

	CustomActorBoundsExtent = FVector::ZeroVector;
	CustomActorBoundsOrigin = FVector::ZeroVector;
	ComponentBoundsBlacklist = { UParticleSystemComponent::StaticClass() };
	bIncludeHiddenComponentsInBounds = false;

	DirectionalLightRotation = FRotator(-45.f, 30.f, 0.f);
	DirectionalLightIntensity = 1.0f;
	DirectionalLightColor = FLinearColor(1.f, 1.f, 1.f);

	DirectionalFillLightRotation = FRotator(-45.f, -160.f, 0.f);
	DirectionalFillLightIntensity = 0.75f;
	DirectionalFillLightColor = FLinearColor(1.f, 1.f, 1.f);

	SkyLightIntensity = 0.8f;
	SkyLightColor = FLinearColor(1.f, 1.f, 1.f);

	bShowEnvironment = true;
	bEnvironmentAffectLighting = true;
	EnvironmentColor = FLinearColor(1.f, 1.f, 1.f);
	EnvironmentCubeMap = FSoftObjectPath("TextureCube'/ThumbnailGenerator/SkySphere/T_Thumbnail_CubeMap.T_Thumbnail_CubeMap'");
	EnvironmentRotation = 0.f;

	PostProcessingSettings = FPostProcessSettings();

	ThumbnailSkySphere = FSoftClassPath("/ThumbnailGenerator/SkySphere/BP_ThumbnailGenerator_SkySphere.BP_ThumbnailGenerator_SkySphere_C");
	ThumbnailGeneratorScripts = { };
}

FThumbnailSettings FThumbnailSettings::MergeThumbnailSettings(const FThumbnailSettings& DefaultSettings, const FThumbnailSettings& OverrideSettings)
{
	FThumbnailSettings OutSettings;

#define MERGE_THUMBNAIL_PROPERTY(MemberName) \
		(void)sizeof(UE4Asserts_Private::GetMemberNameCheckedJunk(((FThumbnailSettings*)0)->MemberName)); \
		if (OverrideSettings.bOverride_##MemberName) { \
			OutSettings.bOverride_##MemberName = true; \
			OutSettings.MemberName = OverrideSettings.MemberName; \
		} \
		else if (DefaultSettings.bOverride_##MemberName) { \
			OutSettings.bOverride_##MemberName = true; \
			OutSettings.MemberName = DefaultSettings.MemberName; \
		}

	MERGE_THUMBNAIL_PROPERTY(ThumbnailTextureWidth);
	MERGE_THUMBNAIL_PROPERTY(ThumbnailTextureHeight);
	MERGE_THUMBNAIL_PROPERTY(ThumbnailBitDepth);
	MERGE_THUMBNAIL_PROPERTY(bCaptureAlpha);
	MERGE_THUMBNAIL_PROPERTY(AlphaBlendMode);

	MERGE_THUMBNAIL_PROPERTY(ProjectionType);
	MERGE_THUMBNAIL_PROPERTY(CameraFitMode);
	MERGE_THUMBNAIL_PROPERTY(CameraFOV);
	MERGE_THUMBNAIL_PROPERTY(OrthoWidthOffset);
	MERGE_THUMBNAIL_PROPERTY(CameraPivot);
	MERGE_THUMBNAIL_PROPERTY(CameraRotation);
	MERGE_THUMBNAIL_PROPERTY(CameraDistanceOffset);
	MERGE_THUMBNAIL_PROPERTY(CameraPositionOffset);
	MERGE_THUMBNAIL_PROPERTY(CameraRotationOffset);

	MERGE_THUMBNAIL_PROPERTY(SimulationMode);
	MERGE_THUMBNAIL_PROPERTY(SimulateSceneTime);
	MERGE_THUMBNAIL_PROPERTY(SimulateSceneFramerate);
	MERGE_THUMBNAIL_PROPERTY(ComponentsToSimulate);

	MERGE_THUMBNAIL_PROPERTY(CustomActorBoundsExtent);
	MERGE_THUMBNAIL_PROPERTY(CustomActorBoundsOrigin);
	MERGE_THUMBNAIL_PROPERTY(ComponentBoundsBlacklist);
	MERGE_THUMBNAIL_PROPERTY(bIncludeHiddenComponentsInBounds);

	MERGE_THUMBNAIL_PROPERTY(DirectionalLightRotation);
	MERGE_THUMBNAIL_PROPERTY(DirectionalLightIntensity);
	MERGE_THUMBNAIL_PROPERTY(DirectionalLightColor);

	MERGE_THUMBNAIL_PROPERTY(DirectionalFillLightRotation);
    MERGE_THUMBNAIL_PROPERTY(DirectionalFillLightIntensity);
    MERGE_THUMBNAIL_PROPERTY(DirectionalFillLightColor);

	MERGE_THUMBNAIL_PROPERTY(SkyLightColor);
	MERGE_THUMBNAIL_PROPERTY(SkyLightIntensity);

	MERGE_THUMBNAIL_PROPERTY(bShowEnvironment);
	MERGE_THUMBNAIL_PROPERTY(bEnvironmentAffectLighting);
	MERGE_THUMBNAIL_PROPERTY(EnvironmentColor);
	MERGE_THUMBNAIL_PROPERTY(EnvironmentCubeMap);
	MERGE_THUMBNAIL_PROPERTY(EnvironmentRotation);

	MERGE_THUMBNAIL_PROPERTY(PostProcessingSettings);

	// Disable auto exposure as it doesn't work well in a thumbnail scenario
	OutSettings.PostProcessingSettings.bOverride_AutoExposureMinBrightness = true;
	OutSettings.PostProcessingSettings.AutoExposureMinBrightness = 1.f;
	OutSettings.PostProcessingSettings.bOverride_AutoExposureMaxBrightness = true;
	OutSettings.PostProcessingSettings.AutoExposureMaxBrightness = 1.f;

	MERGE_THUMBNAIL_PROPERTY(ThumbnailSkySphere);
	MERGE_THUMBNAIL_PROPERTY(ThumbnailGeneratorScripts);

	return OutSettings;
}

UThumbnailGeneratorSettings::UThumbnailGeneratorSettings()
{
	// Force reference some assets to make sure they get cooked
	ConstructorHelpers::FClassFinder<AActor> SkySphereClassFinder(TEXT("/ThumbnailGenerator/SkySphere/BP_ThumbnailGenerator_SkySphere.BP_ThumbnailGenerator_SkySphere_C"));
	AssetRefs.Add(SkySphereClassFinder.Class);

	ConstructorHelpers::FClassFinder<UObject> ThumbnailScriptFinder(TEXT("/ThumbnailGenerator/BP_Thumbnail_CustomDepth_Script.BP_Thumbnail_CustomDepth_Script_C"));
	AssetRefs.Add(ThumbnailScriptFinder.Class);

	ConstructorHelpers::FObjectFinder<UMaterialInterface> ThumbnailOutlineMat1(TEXT("/ThumbnailGenerator/ThumbnailOutline/PP_Thumbnail_Outliner_NoAlpha"));
	AssetRefs.Add(ThumbnailOutlineMat1.Object);

	ConstructorHelpers::FObjectFinder<UMaterialInterface> ThumbnailOutlineMat2(TEXT("/ThumbnailGenerator/ThumbnailOutline/PP_Thumbnail_Outliner_WithAlpha"));
	AssetRefs.Add(ThumbnailOutlineMat2.Object);
}

UThumbnailGeneratorSettings* UThumbnailGeneratorSettings::Get()
{
	return GetMutableDefault<UThumbnailGeneratorSettings>();
}

const TArray<FName> &UThumbnailGeneratorSettings::GetPresetList()
{
	static TArray<FName> PresetList = 
	{
		TEXT("Default"),
		TEXT("Default, No Background"),
		TEXT("Outline"),
		TEXT("Outline, No Background"),
		TEXT("Silhuett"),
		TEXT("Silhuett, No Background")
	};

	return PresetList;
}

void UThumbnailGeneratorSettings::ApplyPreset(FName Preset)
{
	static TMap<FName, TFunction<void()>> PresetMap =
	{
		{ TEXT("Default"), &UThumbnailGeneratorSettings::ApplyDefaultPreset },
		{ TEXT("Default, No Background"), &UThumbnailGeneratorSettings::ApplyDefaultNoBackgroundPreset },
		{ TEXT("Outline"), &UThumbnailGeneratorSettings::ApplyOutlinePreset },
		{ TEXT("Outline, No Background"), &UThumbnailGeneratorSettings::ApplyOutlineNoBackgroundPreset },
		{ TEXT("Silhuett"), &UThumbnailGeneratorSettings::ApplySilhuettPreset },
		{ TEXT("Silhuett, No Background"), &UThumbnailGeneratorSettings::ApplySilhuettNoBackgroundPreset }
	};

	if (auto* PresetFunc = PresetMap.Find(Preset))
		(*PresetFunc)();
}

void UThumbnailGeneratorSettings::ApplyDefaultPreset()
{
	UThumbnailGeneratorSettings* Settings = UThumbnailGeneratorSettings::Get();

	// Don't reset some settigs, as that would be pretty annoying
	const FThumbnailSettings OldSettings = Settings->DefaultThumbnailSettings;

	Settings->DefaultThumbnailSettings = FThumbnailSettings();

	// Restore some old settings
	Settings->DefaultThumbnailSettings.bOverride_ThumbnailTextureWidth = OldSettings.bOverride_ThumbnailTextureWidth;
	Settings->DefaultThumbnailSettings.ThumbnailTextureWidth = OldSettings.ThumbnailTextureWidth;
	
	Settings->DefaultThumbnailSettings.bOverride_ThumbnailTextureHeight = OldSettings.bOverride_ThumbnailTextureHeight;
	Settings->DefaultThumbnailSettings.ThumbnailTextureHeight = OldSettings.ThumbnailTextureHeight;

	Settings->DefaultThumbnailSettings.bOverride_ThumbnailBitDepth = OldSettings.bOverride_ThumbnailBitDepth;
	Settings->DefaultThumbnailSettings.ThumbnailBitDepth = OldSettings.ThumbnailBitDepth;
}

void UThumbnailGeneratorSettings::ApplyDefaultNoBackgroundPreset()
{
	ApplyDefaultPreset();

	UThumbnailGeneratorSettings* Settings = UThumbnailGeneratorSettings::Get();

	// Enable Alpha Capture
	Settings->DefaultThumbnailSettings.bOverride_bCaptureAlpha = true;
	Settings->DefaultThumbnailSettings.bCaptureAlpha = true;

	// Set AlphaBlendMode to Replace
	Settings->DefaultThumbnailSettings.bOverride_AlphaBlendMode = true;
	Settings->DefaultThumbnailSettings.AlphaBlendMode = EThumbnailAlphaBlendMode::EReplace;

	// Disable the Environment
	Settings->DefaultThumbnailSettings.bOverride_bShowEnvironment = true;
	Settings->DefaultThumbnailSettings.bShowEnvironment = false;
}

void UThumbnailGeneratorSettings::ApplyOutlinePreset()
{
	ApplyDefaultPreset();

	UThumbnailGeneratorSettings* Settings = UThumbnailGeneratorSettings::Get();

	// Apply script which turns on custom depth on all primitives
	Settings->DefaultThumbnailSettings.bOverride_ThumbnailGeneratorScripts = true;
	Settings->DefaultThumbnailSettings.ThumbnailGeneratorScripts = { FSoftClassPath("/ThumbnailGenerator/BP_Thumbnail_CustomDepth_Script.BP_Thumbnail_CustomDepth_Script_C").TryLoadClass<UThumbnailGeneratorScript>() };
	
	// Enable post processing override
	Settings->DefaultThumbnailSettings.bOverride_PostProcessingSettings = true;
	
	// Apply the Outline Material
	FPostProcessSettings& PostProcessingSettings = Settings->DefaultThumbnailSettings.PostProcessingSettings;
	PostProcessingSettings.WeightedBlendables.Array = { FWeightedBlendable{ 1.f, FSoftObjectPath("/ThumbnailGenerator/ThumbnailOutline/PP_Thumbnail_Outliner_NoAlpha").TryLoad() } };
}

void UThumbnailGeneratorSettings::ApplyOutlineNoBackgroundPreset()
{
	ApplyDefaultPreset();

	UThumbnailGeneratorSettings* Settings = UThumbnailGeneratorSettings::Get();

	// Enable Alpha Capture
	Settings->DefaultThumbnailSettings.bOverride_bCaptureAlpha = true;
	Settings->DefaultThumbnailSettings.bCaptureAlpha = true;

	// Set AlphaBlendMode to Add
	Settings->DefaultThumbnailSettings.bOverride_AlphaBlendMode = true;
	Settings->DefaultThumbnailSettings.AlphaBlendMode = EThumbnailAlphaBlendMode::EAdd;

	// Disable the Environment
	Settings->DefaultThumbnailSettings.bOverride_bShowEnvironment = true;
	Settings->DefaultThumbnailSettings.bShowEnvironment = false;

	// Apply script which turns on custom depth on all primitives
	Settings->DefaultThumbnailSettings.bOverride_ThumbnailGeneratorScripts = true;
	Settings->DefaultThumbnailSettings.ThumbnailGeneratorScripts = { FSoftClassPath("/ThumbnailGenerator/BP_Thumbnail_CustomDepth_Script.BP_Thumbnail_CustomDepth_Script_C").TryLoadClass<UThumbnailGeneratorScript>() };

	// Enable post processing override
	Settings->DefaultThumbnailSettings.bOverride_PostProcessingSettings = true;

	// Apply the Outline Material
	FPostProcessSettings& PostProcessingSettings = Settings->DefaultThumbnailSettings.PostProcessingSettings;
	PostProcessingSettings.WeightedBlendables.Array = { FWeightedBlendable{ 1.f, FSoftObjectPath("/ThumbnailGenerator/ThumbnailOutline/PP_Thumbnail_Outliner_WithAlpha").TryLoad() } };
}

void UThumbnailGeneratorSettings::ApplySilhuettPreset()
{
	ApplyDefaultPreset();

	UThumbnailGeneratorSettings* Settings = UThumbnailGeneratorSettings::Get();

	// Apply script which turns on custom depth on all primitives
	Settings->DefaultThumbnailSettings.bOverride_ThumbnailGeneratorScripts = true;
	Settings->DefaultThumbnailSettings.ThumbnailGeneratorScripts = { FSoftClassPath("/ThumbnailGenerator/BP_Thumbnail_CustomDepth_Script.BP_Thumbnail_CustomDepth_Script_C").TryLoadClass<UThumbnailGeneratorScript>() };

	// Reset Camera
	Settings->DefaultThumbnailSettings.bOverride_ProjectionType = true;
	Settings->DefaultThumbnailSettings.ProjectionType = ECameraProjectionMode::Orthographic;

	Settings->DefaultThumbnailSettings.bOverride_CameraRotation = true;
	Settings->DefaultThumbnailSettings.CameraRotation = 90.f;

	Settings->DefaultThumbnailSettings.bOverride_CameraPivot = true;
	Settings->DefaultThumbnailSettings.CameraPivot = 0.f;

	// Enable post processing override
	Settings->DefaultThumbnailSettings.bOverride_PostProcessingSettings = true;

	// Apply the Outline Material
	FPostProcessSettings& PostProcessingSettings = Settings->DefaultThumbnailSettings.PostProcessingSettings;
	PostProcessingSettings.WeightedBlendables.Array = { FWeightedBlendable{ 1.f, FSoftObjectPath("/ThumbnailGenerator/Silhuett/PP_Thumbnail_Silhuett").TryLoad() } };
}

void UThumbnailGeneratorSettings::ApplySilhuettNoBackgroundPreset()
{
	ApplySilhuettPreset();

	UThumbnailGeneratorSettings* Settings = UThumbnailGeneratorSettings::Get();

	// Enable Alpha Capture
	Settings->DefaultThumbnailSettings.bOverride_bCaptureAlpha = true;
	Settings->DefaultThumbnailSettings.bCaptureAlpha = true;

	// Set AlphaBlendMode to Replace
	Settings->DefaultThumbnailSettings.bOverride_AlphaBlendMode = true;
	Settings->DefaultThumbnailSettings.AlphaBlendMode = EThumbnailAlphaBlendMode::EReplace;

	// Disable the Environment
	Settings->DefaultThumbnailSettings.bOverride_bShowEnvironment = true;
	Settings->DefaultThumbnailSettings.bShowEnvironment = false;
}

#if WITH_EDITOR
void UThumbnailGeneratorSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName MemberPropertyName = PropertyChangedEvent.MemberProperty ? (PropertyChangedEvent.MemberProperty->GetFName()) : NAME_None;
	
	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UThumbnailGeneratorSettings, BackgroundSceneSettings))
	{
		UThumbnailGenerator::Get()->InvalidateThumbnailWorld();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

// Mans Isaksson 2020

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/GCObject.h"
#include "Runtime/Launch/Resources/Version.h"
#include "ThumbnailGeneratorSettings.h"
#include "ThumbnailGenerator.generated.h"

class UTexture2D;
class UStaticMeshComponent;
class UMaterialInstanceConstant;
class USceneCaptureComponent2D;

class UThumbnailGeneratorScript;

// 4.25 and above has renamed UProperty to FPoperty, the members and overal structure is the same, only its name has changed.
#if ENGINE_MINOR_VERSION < 25 // 4.24 and below
typedef UProperty FProperty;
typedef UArrayProperty FArrayProperty;
typedef UMapProperty FMapProperty;
typedef USetProperty FSetProperty;
typedef UObjectProperty FObjectProperty;
typedef UObjectPropertyBase FObjectPropertyBase;
typedef UDelegateProperty FDelegateProperty;
typedef UMulticastDelegateProperty FMulticastDelegateProperty;
#define FindFProperty FindField
#endif

UENUM(BlueprintType)
enum class ECacheMethod : uint8
{
	// Will always generate a new thumbnail without saving it in the cache.
	EDontUseCache			= 0 UMETA(DisplayName="Ignore cache"),
	// If the thumbnail for the actor has already been generated, this image will be used instead.
	EUseCache				= 1 UMETA(DisplayName="Use cache"),
	// Will always generate a new thumbnail for the actor, and replace the currently cached thumbnail.
	EDontUseCacheSave		= 2 UMETA(DisplayName="Discard previous"),
	// Will not generate a new thumbnail, only looks in cache for existing thumbnail for this actor.
	EUseCacheDontGenerate	= 3 UMETA(DisplayName="Use previous"),
};

DECLARE_DELEGATE_OneParam(FOnThumbnailGenerated, UTexture2D*)

DECLARE_DELEGATE_OneParam(FPreCaptureActorThumbnail, AActor*)

struct FGenerateThumbnailParams
{
	// CacheMethod - How, or if, to use the cache
	ECacheMethod CacheMethod = ECacheMethod::EDontUseCache;

	// This struct can be used to override individual Thumbnail Settings for this capture.
	FThumbnailSettings ThumbnailSettings;

	// Optional pointer to a UTexture2D object to use for the generated thumbnail (if nullptr a new UTexture2D will be created)
	UTexture2D* ResourceObject = nullptr;

	// Optional callback which fires before the thumbnail of the actor is captured
	FPreCaptureActorThumbnail PreCaptureThumbnail;

	// Optional map of properties (PropertyName, PropertyValue) which will be set on the thumbnail actor before capture.
	// See FProperty::ExportTextItem for information on how to serialize a property value to a string.
	TMap<FString, FString> Properties;
};

UCLASS()
class THUMBNAILGENERATOR_API UThumbnailGenerator : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
private:

	TSharedPtr<class FThumbnailSceneInterface> ThumbnailScene;

	TSharedPtr<struct FThumbnailCache>    ThumbnailCache;
	TSharedPtr<struct FRenderTargetCache> RenderTargetCache;

	UPROPERTY()
	class USceneCaptureComponent2D* CaptureComponent = nullptr;

	UPROPERTY()
	TArray<UThumbnailGeneratorScript*> ThumbnailGeneratorScripts;

	TArray<TFunction<void()>> DeferredEventQueue;

public:

	static UThumbnailGenerator* Get();

	void GenerateActorThumbnail(TSubclassOf<AActor> ActorClass, const FOnThumbnailGenerated& Callback, FGenerateThumbnailParams Params = FGenerateThumbnailParams());

	UTexture2D* GenerateActorThumbnail_Synchronous(TSubclassOf<AActor> ActorClass, FGenerateThumbnailParams Params = FGenerateThumbnailParams());

	void InitializeThumbnailWorld(const FThumbnailBackgroundSceneSettings &BackgroundSceneSettings);

	void InvalidateThumbnailWorld();

	UWorld* GetThumbnailWorld() const;

	FORCEINLINE USceneCaptureComponent2D* GetThumbnailCaptureComponent() const { return CaptureComponent;  }

private:

	ESceneCaptureSource GetCaptureSource() const;

	UTexture2D* CaptureThumbnail(const FThumbnailSettings& ThumbnailSettings, UTextureRenderTarget2D* RenderTarget, AActor* Actor, UTexture2D* ResourceObject);

	// ~Begin: FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickableInEditor() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UThumbnailGenerator, STATGROUP_Tickables); }
	// ~End: FTickableGameObject Interface

	// ~Begin: UObject Interface
	virtual void BeginDestroy() override;
	// ~End: UObject Interface
	
	void Shutdown();
	void Cleanup();
	void QueueDeferredTask(const TFunction<void()>& Callback);

};

UCLASS(meta=(ScriptName="ThumbnailGeneration"))
class THUMBNAILGENERATOR_API UThumbnailGeneration : public UObject
{
	GENERATED_BODY()

public:

	/** 
	* Generate a thumbnail for ActorClass synchronousy.
	* @param ActorClass - The actor class of which a thumbnail will be generated.
	* @param CacheMethod - How, or if, to use the cache.
	* @param ThumbnailSettings - This struct can be used to override individual Thumbnail Settings for this capture.
	* @return Pointer to the generated UTexture2D object (null if thumbnail failed to generate)
	*/
	UFUNCTION(BlueprintCallable, Category = "Thumbnail Generator")
	static UTexture2D* GenerateThumbnail_Synchronous(TSubclassOf<AActor> ActorClass, ECacheMethod CacheMethod, FThumbnailSettings ThumbnailSettings);

	/** 
	* Gets the underlying world used for thumbnail generation
	* @return Pointer to the thumbnail UWorld
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Thumbnail Generator")
	static UWorld* GetThumbnailWorld();

	/** 
	* Gets the underlying Capture Component used for thumbnail generation
	* @return Pointer to the USceneCaptureComponent2D
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Thumbnail Generator")
	static USceneCaptureComponent2D* GetThumbnailCaptureComponent();

	/** 
	* Creates the underlying world used for thumbnail generation (Gets called automatically on "Generate Thumbnail"). 
	* Might want to call this if the assets required for thumbnail generation causes hitching when loaded for the first time.
	* 
	* Calling this function multiple times will cause the thumbnail scene to be reconstructed. Use with caution.
	*/
	UFUNCTION(BlueprintCallable, Category = "Thumbnail Generator")
	static void InitializeThumbnailWorld(FThumbnailBackgroundSceneSettings BackgroundSceneSettings);

	/**
	* Saves the UTexture2D thumbnail object to the specified path as a .uasset
	* param Thumbnail - The thumbnail texture to save
	* param Directory - The path to use when saving the thumbnail (needs to be within project content directory)
	* param Name - Optional name to use when saving thumbnail (derived from the Texture object if left blank)
	*/
	UFUNCTION(BlueprintCallable, Category = "Thumbnail Generator|Editor Utility", meta=(DevelopmentOnly))
	static void SaveThumbnail(UTexture2D* Thumbnail, const FDirectoryPath &OutputDirectory, FString OutputName = "");


	// Blueprint Internal Functions

	DECLARE_DYNAMIC_DELEGATE_OneParam(FGenerateThumbnailCallback, class UTexture2D*, Thumbnail);
	DECLARE_DYNAMIC_DELEGATE_OneParam(FPreCaptureThumbnail, class AActor*, Actor);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "TRUE"))
	static void K2_GenerateThumbnail(UClass* ActorClass, ECacheMethod CacheMethod, FThumbnailSettings ThumbnailSettings, 
		TMap<FString, FString> Properties, FGenerateThumbnailCallback Callback, FPreCaptureThumbnail PreCaptureThumbnail);

	UFUNCTION(BlueprintPure, CustomThunk, meta = (CustomStructureParam = "Property", BlueprintInternalUseOnly = "TRUE"))
	static FString K2_ExportPropertyText(const int32& Property);
	DECLARE_FUNCTION(execK2_ExportPropertyText);

	UFUNCTION(BlueprintPure, CustomThunk, meta = (ArrayParm = "ArrayProperty", BlueprintInternalUseOnly = "TRUE"))
	static FString K2_ExportArrayPropertyText(const TArray<int32>& Property);
	DECLARE_FUNCTION(execK2_ExportArrayPropertyText);

	UFUNCTION(BlueprintPure, CustomThunk, meta = (MapParam = "MapProperty", BlueprintInternalUseOnly = "TRUE"))
	static FString K2_ExportMapPropertyText(const TMap<int32, int32>& Property);
	DECLARE_FUNCTION(execK2_ExportMapPropertyText);

	UFUNCTION(BlueprintPure, CustomThunk, meta = (SetParam = "SetProperty", BlueprintInternalUseOnly = "TRUE"))
	static FString K2_ExportSetPropertyText(const TSet<int32>& Property);
	DECLARE_FUNCTION(execK2_ExportSetPropertyText);

};
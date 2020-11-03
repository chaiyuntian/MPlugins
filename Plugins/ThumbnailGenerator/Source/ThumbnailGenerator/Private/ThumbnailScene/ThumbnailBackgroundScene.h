// Mans Isaksson 2020

#pragma once
#include "ThumbnailSceneInterface.h"
#include "ThumbnailGeneratorSettings.h"
#include "UObject/GCObject.h"
#include "Engine/Engine.h"
#include "ThumbnailBackgroundScene.generated.h"

UCLASS()
class UBackgroundLevelStreamingFixer : public UObject
{
	GENERATED_BODY()
private:
	
	UPROPERTY()
	class ULevelStreaming* LevelStreaming;

	int32 InstanceID;

public:
	void SetStreamingLevel(ULevelStreaming* InLevelStreaming, int32 InInstanceID);

	UFUNCTION()
	void OnLevelShown();
};

class FThumbnailBackgroundScene : public FThumbnailSceneInterface, public FGCObject
{
private:
	class UDirectionalLightComponent* DirectionalLight     = nullptr;
	class UDirectionalLightComponent* DirectionalFillLight = nullptr;
	class USkyLightComponent*         SkyLight             = nullptr;
	class AActor*                     SkySphereActor       = nullptr;

	class UWorld* BackgroundWorld = nullptr;

	FLinearColor LastEnvironmentColor = FLinearColor::White;

	FThumbnailBackgroundSceneSettings SceneSettings;

	struct FInstanceID // Simple class for generating IDs with a preference for lower values
	{
	private:
		static TSet<int32>& TakenIDs() { static TSet<int32> IDs; return IDs; }
		int32 UniqueID;

	public:
		FInstanceID()
		{
			int32 ID = 0;
			while (true)
			{
				if (!TakenIDs().Contains(ID))
				{
					UniqueID = ID;
					TakenIDs().Add(UniqueID);
					break;
				}
				ID++;
			}
		}

		~FInstanceID() { TakenIDs().Remove(UniqueID); }

		FORCEINLINE int32 GetID() const { return UniqueID; }
	} InstanceID;

public:

	FThumbnailBackgroundScene(const FThumbnailBackgroundSceneSettings &BackgroundSceneSettings);
	virtual ~FThumbnailBackgroundScene();
	
	FWorldContext* GetWorldContext() const;

	/** Begin FThumbnailSceneInterface */
	virtual void UpdateScene(const FThumbnailSettings& ThumbnailSettings, bool bForceUpdate = false) override;
	virtual class UWorld* GetThumbnailWorld() const override { return BackgroundWorld; };
	/** End FThumbnailSceneInterface*/

	// ~Begin: FGCObject Interface
	void AddReferencedObjects(FReferenceCollector& Collector) override;
	// ~End: FGCObject Interface
};
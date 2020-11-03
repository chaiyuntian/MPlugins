// Mans Isaksson 2020

#include "ThumbnailGenerator.h"
#include "ThumbnailGeneratorModule.h"
#include "ThumbnailGeneratorInterfaces.h"
#include "ThumbnailGeneratorScript.h"
#include "ThumbnailScene/ThumbnailPreviewScene.h"
#include "ThumbnailScene/ThumbnailBackgroundScene.h"
#include "CacheProvider.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/SkeletalMeshRenderData.h"

#include "Framework/Application/SlateApplication.h"
#include "HAL/Platform.h"

#include "TimerManager.h"
#include "FXSystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "GameDelegates.h"

struct FThumbnailCache : public TCacheProvider<UClass*, UTexture2D>
{
	virtual int32 MaxCacheSize() override { return UThumbnailGeneratorSettings::Get()->MaxThumbnailCacheSize * 1000 * 1000; }
	virtual int32 GetItemDataFootprint(UTexture2D* InTexture) override { return InTexture->PlatformData->Mips[0].BulkData.GetBulkDataSize(); }
	virtual FString DebugCacheName() override { return TEXT("Thumbnail Cache"); }

	// We don't do anything with textures that gets ejected from the cache as they are now "Owned" by the user,
	// we cannot know whether they're still using the thumbnail.
	virtual void OnItemRemovedFromCache(UTexture2D* InObject) {}
};

struct FHashableRenderTargetInfo
{
	uint16 Width  = 0;
	uint16 Height = 0;
	EThumbnailBitDepth BitDepth = EThumbnailBitDepth::E8;
	friend inline uint32 GetTypeHash(const FHashableRenderTargetInfo& O) 
	{
		return (uint32(O.Width) << 16) // first 16 bits
			| (uint32(O.Height) & 0xfffffffe) // 17-31
			| (O.BitDepth == EThumbnailBitDepth::E8 ? 0 : 1); // 32:nd bit
	}
	friend inline bool operator==(const FHashableRenderTargetInfo& A, const FHashableRenderTargetInfo& B) { return GetTypeHash(A) == GetTypeHash(B); }
};

struct FRenderTargetCache : public TCacheProvider<FHashableRenderTargetInfo, UTextureRenderTarget2D>
{
	virtual int32 MaxCacheSize() override { return UThumbnailGeneratorSettings::Get()->MaxRenderTargetCacheSize * 1000 * 1000; }
	virtual int32 GetItemDataFootprint(UTextureRenderTarget2D* InRenderTarget) override 
	{ 
		const int32 BytesPerPixel = [&]() 
		{
			switch (InRenderTarget->GetFormat())
			{
			case PF_B8G8R8A8: return 4;
			case PF_FloatRGBA: return 8;
			}
			return 4; // Unsuported format, default to 1 byte per component
		}();
		return InRenderTarget->SizeX * InRenderTarget->SizeY * BytesPerPixel;
	}
	virtual FString DebugCacheName() override { return TEXT("Render Target Cache"); }

	virtual void OnItemRemovedFromCache(UTextureRenderTarget2D* InRenderTarget) { InRenderTarget->MarkPendingKill(); }
};

UThumbnailGenerator* UThumbnailGenerator::Get()
{
	static TWeakObjectPtr<UThumbnailGenerator> Instance;
	if (Instance.IsValid())
		return Instance.Get();

	Instance = NewObject<UThumbnailGenerator>((UObject*)GetTransientPackage(), TEXT("Thumbnail Generator"));
	if (Instance.IsValid())
	{
		FGameDelegates::Get().GetExitCommandDelegate().AddUObject(Instance.Get(), &UThumbnailGenerator::Shutdown);
		FGameDelegates::Get().GetEndPlayMapDelegate().AddUObject(Instance.Get(), &UThumbnailGenerator::Shutdown);
		Instance->AddToRoot();
	}

	return Instance.Get();
}

void UThumbnailGenerator::GenerateActorThumbnail(TSubclassOf<AActor> ActorClass, const FOnThumbnailGenerated& Callback, FGenerateThumbnailParams Params)
{
	QueueDeferredTask([ActorClass, Callback, Params]()
	{
		if (UThumbnailGenerator* ThumbnailGenerator = UThumbnailGenerator::Get())
			Callback.ExecuteIfBound(ThumbnailGenerator->GenerateActorThumbnail_Synchronous(ActorClass, Params));
	});
}

UTexture2D* UThumbnailGenerator::GenerateActorThumbnail_Synchronous(TSubclassOf<AActor> ActorClass, FGenerateThumbnailParams Params)
{
	const auto EjectWithError = [&](const FString &Error) 
	{
		const FString FuncName = TEXT("UThumbnailGenerator::GenerateActorThumbnail_Internal");
		UE_LOG(LogThumbnailGenerator, Error, TEXT("%s - %s"), *FuncName , *Error);
		return nullptr;
	};

	UClass* const ClassPtr = ActorClass.Get();

	if (!IsValid(ClassPtr))
		return EjectWithError("Invalid Actor Class");

	if (Params.ThumbnailSettings.ThumbnailTextureWidth <= 0 || Params.ThumbnailSettings.ThumbnailTextureHeight <= 0)
		return EjectWithError(FString::Printf(TEXT("Invalid Texture Size (%dx%d)"), Params.ThumbnailSettings.ThumbnailTextureWidth, Params.ThumbnailSettings.ThumbnailTextureHeight));

	if (!ThumbnailScene.IsValid())
		InitializeThumbnailWorld(UThumbnailGeneratorSettings::Get()->BackgroundSceneSettings);

	switch (Params.CacheMethod)
	{
	case ECacheMethod::EUseCache:
		if (UTexture2D* CachedThumbnail = ThumbnailCache->GetCachedItem(ClassPtr))
			return CachedThumbnail;
		break;
	case ECacheMethod::EUseCacheDontGenerate:
		if (UTexture2D* CachedThumbnail = ThumbnailCache->GetCachedItem(ClassPtr))
			return CachedThumbnail;
		else
			return nullptr;
		break;
	default:
		break;
	}

	UWorld* const ThumbnailWorld = ThumbnailScene->GetThumbnailWorld();
	if (!IsValid(ThumbnailWorld))
		return EjectWithError("Invalid Preview World");


	// Update scripts
	{
		const auto AreScriptsDifferent = [](const TArray<UThumbnailGeneratorScript*> &ExistingScripts, const TArray<TSubclassOf<UThumbnailGeneratorScript>> &NewScripts)->bool
		{
			if (ExistingScripts.Num() != NewScripts.Num())
				return true;

			// Make order matter since we expect this array not to change much. It is therefore 
			// more important that the compare function is fast, rather than we avoid re-creating the ThumbnailGeneratorScripts
			for (int32 i = 0; i < NewScripts.Num(); i++)
			{
				if (!IsValid(ExistingScripts[i]) || NewScripts[i].Get() != ExistingScripts[i]->GetClass())
					return true;
			}

			return false;
		};

		if (AreScriptsDifferent(ThumbnailGeneratorScripts, Params.ThumbnailSettings.ThumbnailGeneratorScripts))
		{
			for (UThumbnailGeneratorScript* ThumbnailGeneratorScript : ThumbnailGeneratorScripts)
				if (IsValid(ThumbnailGeneratorScript))
					ThumbnailGeneratorScript->MarkPendingKill();

			ThumbnailGeneratorScripts.Empty();

			for (const TSubclassOf<UThumbnailGeneratorScript>& ThumbnailGeneratorScript : Params.ThumbnailSettings.ThumbnailGeneratorScripts)
			{
				if (ThumbnailGeneratorScript.Get())
					ThumbnailGeneratorScripts.Add(NewObject<UThumbnailGeneratorScript>(this, ThumbnailGeneratorScript.Get()));
			}
		}
	}

	ThumbnailScene->UpdateScene(Params.ThumbnailSettings);

	FActorSpawnParameters SpawnParams;
	SpawnParams.bNoFail                        = true;
	SpawnParams.bDeferConstruction             = true;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* const SpawnedActor = ThumbnailWorld->SpawnActor<AActor>(ActorClass.Get(), SpawnParams);

	if (!IsValid(SpawnedActor))
		return EjectWithError("Failed to spawn thumbnail actor");

	UClass* SpawnedActorClass = SpawnedActor->GetClass();
	for (const TPair<FString, FString>& SerializedProperty : Params.Properties)
	{
		if (FProperty* Property = FindFProperty<FProperty>(SpawnedActorClass, *SerializedProperty.Key))
			Property->ImportText(*SerializedProperty.Value, Property->ContainerPtrToValuePtr<void>(SpawnedActor), 0, SpawnedActor);
	}

	SpawnedActor->FinishSpawning(FTransform::Identity);

	if (SpawnedActor->Implements<UThumbnailActorInterface>())
	{
		FTransform ThumbnailActorTransform = IThumbnailActorInterface::Execute_GetThumbnailTransform(SpawnedActor);
		if (!IsValid(SpawnedActor)) 
			return EjectWithError("IThumbnailActorInterface::GetThumbnailTransform has destroyed the thumbnail actor");

		SpawnedActor->SetActorTransform(ThumbnailActorTransform);

		IThumbnailActorInterface::Execute_PreCaptureActorThumbnail(SpawnedActor);
		if (!IsValid(SpawnedActor)) 
			return EjectWithError("IThumbnailActorInterface::PreCaptureActorThumbnail has destroyed the thumbnail actor");
	}

	for (UThumbnailGeneratorScript* ThumbnailGeneratorScript : ThumbnailGeneratorScripts)
	{
		ThumbnailGeneratorScript->PreCaptureActorThumbnail(SpawnedActor);
		if (!IsValid(SpawnedActor)) 
			return EjectWithError("UThumbnailGeneratorScript::PreCaptureActorThumbnail has destroyed the thumbnail actor");
	}

	Params.PreCaptureThumbnail.ExecuteIfBound(SpawnedActor);

	if (!IsValid(SpawnedActor)) 
		return EjectWithError("PreCaptureThumbnail Event has destroyed the thumbnail actor");

	// Simulate scene
	{
		const auto GetActorComponents = [&]()
		{
			TArray<UActorComponent*> Components;
			SpawnedActor->GetComponents(Components, true);
			return Components;
		};

		const auto SimulatedTick = [&Params](const TFunction<void(float)>& TickCallback)
		{
			const auto StepSize = 1.f / Params.ThumbnailSettings.SimulateSceneFramerate;
			for (float time = Params.ThumbnailSettings.SimulateSceneTime; time > 0.f; time -= StepSize)
			{
				const auto dt = StepSize + FMath::Min(0.f, time - StepSize);
				TickCallback(dt);
			}
		};

		const auto DispatchComponentsBeginPlay = [](const TArray<UActorComponent*> &Components)
		{
			for (UActorComponent* Component : Components)
			{
				if (Component->IsRegistered() && !Component->HasBegunPlay())
				{
					Component->RegisterAllComponentTickFunctions(true);
					Component->BeginPlay();

					if (UParticleSystemComponent* ParticleSystemComponent = Cast<UParticleSystemComponent>(Component))
						ParticleSystemComponent->bWarmingUp = true; // To prevent async updates
				}
			}
		};

		switch (Params.ThumbnailSettings.SimulationMode)
		{
		case EThumbnailSceneSimulationMode::EActor:
		{
			const auto SpawnedComponents = GetActorComponents();

			// Do the component dispatch ourselves as we need to set some custom settings anyway
			DispatchComponentsBeginPlay(SpawnedComponents);

			SpawnedActor->DispatchBeginPlay();

			SimulatedTick([&](float DeltaTime)
			{
				for (UActorComponent* Component : SpawnedComponents)
				{
					if (Component->IsRegistered() && Component->HasBegunPlay())
						Component->TickComponent(DeltaTime, LEVELTICK_All, &Component->PrimaryComponentTick);
				}

				SpawnedActor->TickActor(DeltaTime, LEVELTICK_All, SpawnedActor->PrimaryActorTick);
			});
		}

		case EThumbnailSceneSimulationMode::EAllComponents:
		{
			const auto SpawnedComponents = GetActorComponents();
			DispatchComponentsBeginPlay(SpawnedComponents);

			SimulatedTick([&](float DeltaTime)
			{
				for (UActorComponent* Component : SpawnedComponents)
				{
					if (Component->IsRegistered() && Component->HasBegunPlay())
						Component->TickComponent(DeltaTime, LEVELTICK_All, &Component->PrimaryComponentTick);
				}
			});
		}

		case EThumbnailSceneSimulationMode::ESpecifiedComponents:
		{
			const auto IsTickable = [&](UActorComponent* Component)
			{
				for (const auto& ComponentClass : Params.ThumbnailSettings.ComponentsToSimulate)
				{
					if (Component->IsA(ComponentClass.Get()))
						return true;
				}
				return false;
			};

			const auto SpawnedComponents = GetActorComponents().FilterByPredicate([&](auto* Component) { return IsTickable(Component); });
			DispatchComponentsBeginPlay(SpawnedComponents);

			SimulatedTick([&](float DeltaTime)
			{
				for (UActorComponent* Component : SpawnedComponents)
				{
					if (Component->IsRegistered() && Component->HasBegunPlay())
						Component->TickComponent(DeltaTime, LEVELTICK_All, &Component->PrimaryComponentTick);
				}
			});
		}

		case EThumbnailSceneSimulationMode::ENone:
		default:
			break;
		}
	}

	struct Local
	{
		static UTextureRenderTarget2D* CreateTextureTarget(UObject* Outer, int32 Width, int32 Height, ETextureRenderTargetFormat Format, const FLinearColor &ClearColor)
		{
			if (Width > 0 && Height > 0)
			{
				static uint32 count = 0;
				UTextureRenderTarget2D* NewRenderTarget2D = NewObject<UTextureRenderTarget2D>(Outer, *FString::Printf(TEXT("ThumbnailRenderTarget_%d_%dx%d"), ++count, Width, Height), RF_Transient);
				NewRenderTarget2D->RenderTargetFormat = Format;
				NewRenderTarget2D->ClearColor = ClearColor;
				NewRenderTarget2D->InitAutoFormat(Width, Height);
				NewRenderTarget2D->UpdateResourceImmediate(true);

				return NewRenderTarget2D;
			}

			return nullptr;
		}
	};

	const auto RenderTargetWidth  = uint16(Params.ThumbnailSettings.ThumbnailTextureWidth);
	const auto RenderTargetHeight = uint16(Params.ThumbnailSettings.ThumbnailTextureHeight);
	const auto RenderBitDepth     = Params.ThumbnailSettings.ThumbnailBitDepth;
	const auto RenderTargetInfo   = FHashableRenderTargetInfo{ RenderTargetWidth, RenderTargetHeight, RenderBitDepth };
	UTextureRenderTarget2D* RenderTarget = RenderTargetCache->GetCachedItem(RenderTargetInfo);
	if (!RenderTarget)
	{
		RenderTarget = Local::CreateTextureTarget(
			GetTransientPackage(),
			RenderTargetWidth,
			RenderTargetHeight,
			RenderBitDepth == EThumbnailBitDepth::E8 ? ETextureRenderTargetFormat::RTF_RGBA8_SRGB : ETextureRenderTargetFormat::RTF_RGBA16f,
			FLinearColor(0.f, 0.f, 0.f, 1.f) // Important: When rendering with MSAA the alpha will no be touched, setting it as 0 would leave the whole image fully transparent
		);

		if (!ensure(RenderTarget))
			return nullptr;

		RenderTargetCache->CacheItem(RenderTargetInfo, RenderTarget);
	}

	UTexture2D* const Thumbnail = CaptureThumbnail(Params.ThumbnailSettings, RenderTarget, SpawnedActor, Params.ResourceObject);
	if (!Thumbnail)
	{
		UE_LOG(LogThumbnailGenerator, Error, TEXT("Failed to generate thumbnail texture"));
		return nullptr;
	}

	switch (Params.CacheMethod)
	{
	case ECacheMethod::EUseCache:
		ThumbnailCache->CacheItem(ActorClass, Thumbnail);
		break;
	case ECacheMethod::EDontUseCacheSave:
		ThumbnailCache->CacheItem(ActorClass, Thumbnail);
		break;
	default:
		break;
	}

	SpawnedActor->Destroy();

	return Thumbnail;
}

void UThumbnailGenerator::InitializeThumbnailWorld(const FThumbnailBackgroundSceneSettings &BackgroundSceneSettings)
{
	if (ThumbnailScene.IsValid())
		ThumbnailScene.Reset();

	UWorld* ThumbnailWorld = nullptr;

	if (BackgroundSceneSettings.BackgroundWorld.ToSoftObjectPath().IsValid())
	{
		ThumbnailScene = MakeShareable(new FThumbnailBackgroundScene(BackgroundSceneSettings));

		ThumbnailWorld = ThumbnailScene->GetThumbnailWorld();
		checkf(ThumbnailWorld, TEXT("Could not create thumbnail background world"));
	}
	else
	{
		ThumbnailScene = MakeShareable(new FThumbnailPreviewScene);
		ThumbnailWorld = ThumbnailScene->GetThumbnailWorld();
	}
	

	CaptureComponent = NewObject<USceneCaptureComponent2D>(GetTransientPackage());
	CaptureComponent->bCaptureEveryFrame           = false;
	CaptureComponent->bCaptureOnMovement           = false;
	CaptureComponent->PostProcessBlendWeight       = 1.f;
	CaptureComponent->PrimitiveRenderMode          = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	CaptureComponent->CompositeMode                = ESceneCaptureCompositeMode::SCCM_Overwrite;
	CaptureComponent->CaptureSource                = GetCaptureSource();
	#if ENGINE_MINOR_VERSION > 24
	CaptureComponent->bDisableFlipCopyGLES         = false;
	#endif
	CaptureComponent->bAlwaysPersistRenderingState = true;
	CaptureComponent->TextureTarget                = nullptr;
	CaptureComponent->bConsiderUnrenderedOpaquePixelAsFullyTranslucent = true;

	CaptureComponent->RegisterComponentWithWorld(ThumbnailWorld);


	if (!ThumbnailCache.IsValid())
		ThumbnailCache = MakeShareable(new FThumbnailCache);

	if (!RenderTargetCache.IsValid())
		RenderTargetCache = MakeShareable(new FRenderTargetCache);
}

void UThumbnailGenerator::InvalidateThumbnailWorld()
{
	if (IsValid(CaptureComponent))
	{
		CaptureComponent->DestroyComponent();
		CaptureComponent = nullptr;
	}

	if (ThumbnailScene.IsValid())
		ThumbnailScene.Reset();
}

UWorld* UThumbnailGenerator::GetThumbnailWorld() const
{
	return ThumbnailScene.IsValid() ? ThumbnailScene->GetThumbnailWorld() : nullptr;
}

ESceneCaptureSource UThumbnailGenerator::GetCaptureSource() const
{
	return IsFeatureLevelSupported(GMaxRHIShaderPlatform, ERHIFeatureLevel::SM5) ? ESceneCaptureSource::SCS_FinalColorHDR : ESceneCaptureSource::SCS_FinalColorLDR;
}

UTexture2D* UThumbnailGenerator::CaptureThumbnail(const FThumbnailSettings& ThumbnailSettings, UTextureRenderTarget2D* RenderTarget, AActor* Actor, UTexture2D* ResourceObject)
{
	struct Local
	{
		static TArray<uint8> ExtractAlpha(UTextureRenderTarget2D* TextureTarget, bool bInverseAlpha)
		{
			FRenderTarget* const TextureRenderTarget = TextureTarget->GameThread_GetRenderTargetResource();

			const EPixelFormat PixelFormat = TextureTarget->GetFormat();
			const ETextureSourceFormat TextureFormat = [&]() {
				switch (PixelFormat)
				{
				case PF_B8G8R8A8: return TSF_BGRA8;
				case PF_FloatRGBA: return TSF_RGBA16F;
				}
				return TSF_Invalid;
			}();

			TArray<uint8> OutAlpha;

			if (TextureFormat == TSF_BGRA8)
			{
				TArray<FColor> SurfData;
				TextureRenderTarget->ReadPixels(SurfData);

				OutAlpha.Reserve(SurfData.Num());

				for (const FColor &Data : SurfData)
					OutAlpha.Add(bInverseAlpha ? 255 - Data.A : Data.A);
			}
			else if (TextureFormat == TSF_RGBA16F)
			{
				TArray<FFloat16Color> SurfData;
				TextureRenderTarget->ReadFloat16Pixels(SurfData);

				OutAlpha.Reserve(SurfData.Num());

				for (const FFloat16Color& Data : SurfData)
				{
					const uint8 Alpha = (float)(Data.A) * 255.f;
					OutAlpha.Add(bInverseAlpha ? 255 - Alpha : Alpha);
				}
			}

			return OutAlpha;
		}

		static UTexture2D* ConstructTransientTexture2D(UObject* Outer, const FString& NewTexName, uint32 SizeX, uint32 SizeY, EPixelFormat PixelFormat)
		{
			const bool bIsValidSize = (SizeX != 0 && SizeY != 0);
			if (!bIsValidSize)
			{
				UE_LOG(LogThumbnailGenerator, Error, TEXT("FThumbnailPreviewScene::ConstructTransientTexture2D - Invalid Texture Size: %dx%d"), SizeX, SizeY)
				return nullptr;
			}

			const bool bIsValidFormat = [&]() // Right now, we only support RGBA 8 and 16 bit
			{
				switch (PixelFormat)
				{
				case PF_B8G8R8A8: return true;
				case PF_FloatRGBA: return true;
				}
				return false;
			}();

			if (!bIsValidFormat)
			{
				UE_LOG(LogThumbnailGenerator, Error, TEXT("FThumbnailPreviewScene::ConstructTransientTexture2D - Invalid Texture Format"))
				return nullptr;
			}

			// create the 2d texture
			UTexture2D* Result = UTexture2D::CreateTransient(SizeX, SizeY, PixelFormat, *NewTexName);
			Result->NeverStream = true;
			Result->VirtualTextureStreaming = false;

			return Result;
		}

		static void FillTextureData(UTexture2D* Texture2D, UTextureRenderTarget2D* TextureTarget, const TArray<uint8>* AlphaOverride, EThumbnailAlphaBlendMode AlphaBlendMode, uint32 Flags)
		{
			FRenderTarget* const TextureRenderTarget = TextureTarget->GameThread_GetRenderTargetResource();
			if (!TextureRenderTarget)
			{
				return;
			}

			const auto PixelFormat = TextureTarget->GetFormat();

			if (Texture2D->GetSizeX() != TextureTarget->SizeX || Texture2D->GetSizeY() != TextureTarget->SizeY || Texture2D->GetPixelFormat() != PixelFormat)
			{
				UE_LOG(LogThumbnailGenerator, Log, TEXT("Resize Texture2D %s to fit dimentions %dx%d"), *Texture2D->GetName(), TextureTarget->SizeX, TextureTarget->SizeY);

				const ETextureSourceFormat TextureFormat = [&]() // Right now, we only support RGBA 8 and 16 bit
				{
					switch (TextureTarget->GetFormat())
					{
					case PF_B8G8R8A8: return ETextureSourceFormat::TSF_RGBA8;
					case PF_FloatRGBA: return ETextureSourceFormat::TSF_RGBA16;
					}
					return ETextureSourceFormat::TSF_Invalid;
				}();

				if (TextureFormat == ETextureSourceFormat::TSF_Invalid)
				{
					UE_LOG(LogThumbnailGenerator, Error, TEXT("FThumbnailPreviewScene::FillTextureData - Tried to fill with invalid texture format!"));
					return;
				}

				Texture2D->ReleaseResource();

				if (!Texture2D->PlatformData)
				{
					Texture2D->PlatformData = new FTexturePlatformData();
				}

				if (Texture2D->PlatformData->Mips.Num() == 0)
				{
					Texture2D->PlatformData->Mips.Add(new FTexture2DMipMap());
				}
					
				FTexture2DMipMap& Mip = Texture2D->PlatformData->Mips[0];
					
				Texture2D->PlatformData->SizeX = TextureTarget->SizeX;
				Texture2D->PlatformData->SizeY = TextureTarget->SizeY;
				Texture2D->PlatformData->PixelFormat = TextureTarget->GetFormat();

				const auto BlockSize = TextureFormat == ETextureSourceFormat::TSF_RGBA8 ? sizeof(FColor) : sizeof(FFloat16Color);
				Mip.SizeX = TextureTarget->SizeX;
				Mip.SizeY = TextureTarget->SizeY;
				Mip.BulkData.Lock(LOCK_READ_WRITE);
				Mip.BulkData.Realloc(TextureTarget->SizeX * TextureTarget->SizeY * BlockSize);
				Mip.BulkData.Unlock();
			}

			FTexture2DMipMap& mip = Texture2D->PlatformData->Mips[0];
			uint32* const TextureData = (uint32*)mip.BulkData.Lock(LOCK_READ_WRITE);
			const int32 TextureDataSize = mip.BulkData.GetBulkDataSize();

			const auto MixAlpha = [&](uint8 A1, uint8 A2)->uint8
			{
				switch (AlphaBlendMode)
				{
				case EThumbnailAlphaBlendMode::EReplace:
					return A2;
				case EThumbnailAlphaBlendMode::EAdd:
					return A1 + A2;
				case EThumbnailAlphaBlendMode::EMultiply:
					return A1 * A2;
				case EThumbnailAlphaBlendMode::ESubract:
					return A1 - A2;
				}
				return A2;
			};

			// read the 2d surface
			if (PixelFormat == PF_B8G8R8A8)
			{
				TArray<FColor> SurfData;
				TextureRenderTarget->ReadPixels(SurfData);
				// override the alpha if desired
				if (AlphaOverride)
				{
					check(SurfData.Num() == AlphaOverride->Num());
					for (int32 Pixel = 0; Pixel < SurfData.Num(); Pixel++)
					{
						SurfData[Pixel].A = MixAlpha(SurfData[Pixel].A, (*AlphaOverride)[Pixel]);
					}
				}

				// copy the 2d surface data to the first mip of the static 2d texture
				check(TextureDataSize == SurfData.Num() * sizeof(FColor));
				FMemory::Memcpy(TextureData, SurfData.GetData(), TextureDataSize);
			}
			else if (PixelFormat == PF_FloatRGBA)
			{
				TArray<FFloat16Color> SurfData;
				TextureRenderTarget->ReadFloat16Pixels(SurfData);
				// override the alpha if desired
				if (AlphaOverride)
				{
					check(SurfData.Num() == AlphaOverride->Num());
					for (int32 Pixel = 0; Pixel < SurfData.Num(); Pixel++)
					{
						SurfData[Pixel].A = ((float)MixAlpha(uint8(SurfData[Pixel].A * 255.f), (*AlphaOverride)[Pixel])) / 255.0f;
					}
				}

				// copy the 2d surface data to the first mip of the static 2d texture
				check(TextureDataSize == SurfData.Num() * sizeof(FFloat16Color));
				FMemory::Memcpy(TextureData, SurfData.GetData(), TextureDataSize);
			}

			mip.BulkData.Unlock();

			// if render target gamma used was 1.0 then disable SRGB for the static texture
			if (FMath::Abs(TextureRenderTarget->GetDisplayGamma() - 1.0f) < KINDA_SMALL_NUMBER)
			{
				Flags &= ~CTF_SRGB;
			}

			Texture2D->SRGB = (Flags & CTF_SRGB) != 0;

			Texture2D->UpdateResource();
		}
	};

	const auto CameraRotation = [&]()
	{
		const FQuat Yaw = FQuat(FVector::UpVector, FMath::DegreesToRadians(ThumbnailSettings.CameraRotation));
		const FQuat Pivot = FQuat(Yaw.GetRightVector(), FMath::DegreesToRadians(ThumbnailSettings.CameraPivot));
		return ThumbnailSettings.CameraRotationOffset.Quaternion() * Pivot * Yaw;
	}();

	const auto GetActorBoundingBox = [&ThumbnailSettings](AActor* InActor)->FBox
	{
		const auto GetPrimitiveBounds = [&ThumbnailSettings](UPrimitiveComponent* InPrimitiveComponent)
		{
			if (InPrimitiveComponent->bUseAttachParentBound && InPrimitiveComponent->GetAttachParent() != nullptr)
				return FBox(EForceInit::ForceInit);

			const auto GetSkinnedMeshBounds = [](USkinnedMeshComponent* SkinnedMeshComponent)
			{
				const auto LODIndex = 0;

				const USkeletalMesh* SkeletalMesh = SkinnedMeshComponent->SkeletalMesh;
				if (!IsValid(SkeletalMesh) 
					|| !SkeletalMesh->GetResourceForRendering()
					|| !SkeletalMesh->GetResourceForRendering()->LODRenderData.IsValidIndex(LODIndex))
					return SkinnedMeshComponent->CalcBounds(SkinnedMeshComponent->GetComponentTransform()).GetBox();

				TArray<FVector> VertexPositions;
				TArray<FMatrix> CachedRefToLocals;
				SkinnedMeshComponent->CacheRefToLocalMatrices(CachedRefToLocals); 

				const FSkeletalMeshLODRenderData& SkelMeshLODData = SkeletalMesh->GetResourceForRendering()->LODRenderData[LODIndex];
				const FSkinWeightVertexBuffer* SkinWeightBuffer = SkinnedMeshComponent->GetSkinWeightBuffer(LODIndex);
				if (!SkinWeightBuffer)
					return SkinnedMeshComponent->CalcBounds(SkinnedMeshComponent->GetComponentTransform()).GetBox();

				USkinnedMeshComponent::ComputeSkinnedPositions(SkinnedMeshComponent, VertexPositions, CachedRefToLocals, SkelMeshLODData, *SkinWeightBuffer);

				FBox Bounds(EForceInit::ForceInit);
				for (const auto& Position : VertexPositions)
					Bounds += Position;

				return Bounds.TransformBy(SkinnedMeshComponent->GetComponentTransform());
			};

			if (!ThumbnailSettings.bIncludeHiddenComponentsInBounds && !InPrimitiveComponent->IsVisible())
				return FBox(EForceInit::ForceInit);

			const auto bIsBlacklisted = [&]()
			{
				for (UClass* BlacklistedClass : ThumbnailSettings.ComponentBoundsBlacklist)
				{
					if (InPrimitiveComponent->IsA(BlacklistedClass))
						return true;
				}
				return false;
			}();

			if (bIsBlacklisted)
				return FBox(EForceInit::ForceInit);

			if (USkinnedMeshComponent* SkinnedMeshComponent = Cast<USkeletalMeshComponent>(InPrimitiveComponent)) 
				return GetSkinnedMeshBounds(SkinnedMeshComponent);

			if (UParticleSystemComponent* ParticleSystemComponent = Cast<UParticleSystemComponent>(InPrimitiveComponent))
				return FBox(EForceInit::ForceInit);

			return InPrimitiveComponent->CalcBounds(InPrimitiveComponent->GetComponentTransform()).GetBox();
		};

		FBox Box(EForceInit::ForceInit);
		for (UActorComponent* ActorComponent : InActor->GetComponents())
		{
			UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(ActorComponent);
			if (PrimComp && PrimComp->IsRegistered())
				Box += GetPrimitiveBounds(PrimComp);
		}

		return Box;
	};

	const FBox BoundingBox = (!ThumbnailSettings.bOverride_CustomActorBoundsOrigin || !ThumbnailSettings.bOverride_CustomActorBoundsExtent)
		? GetActorBoundingBox(Actor)
		: FBox(EForceInit::ForceInitToZero);

	FVector BoundsOrigin = ThumbnailSettings.bOverride_CustomActorBoundsOrigin
		? ThumbnailSettings.CustomActorBoundsOrigin
		: BoundingBox.GetCenter();

	const FVector BoundsExtent = ThumbnailSettings.bOverride_CustomActorBoundsExtent
		? ThumbnailSettings.CustomActorBoundsExtent
		: BoundingBox.GetExtent();

	// Adjust the Z location to snap the actor to the "floor" plane
	{
		const float ActorZOffset = BoundsOrigin.Z - BoundsExtent.Z;
		BoundsOrigin.Z -= ActorZOffset;

		FVector ActorLocaton = Actor->GetActorLocation();
		Actor->SetActorLocation(FVector(ActorLocaton.X, ActorLocaton.Y, ActorLocaton.Z - ActorZOffset));
	}

	const auto BoundsMin = BoundsOrigin - BoundsExtent;
	const auto BoundsMax = BoundsOrigin + BoundsExtent;

	const TArray<FVector, TInlineAllocator<8>> BoundsVertices =
	{
		{ BoundsMin.X, BoundsMin.Y, BoundsMin.Z },
		{ BoundsMin.X, BoundsMin.Y, BoundsMax.Z },
		{ BoundsMin.X, BoundsMax.Y, BoundsMax.Z },
		{ BoundsMax.X, BoundsMax.Y, BoundsMax.Z },
		{ BoundsMax.X, BoundsMax.Y, BoundsMin.Z },
		{ BoundsMax.X, BoundsMin.Y, BoundsMin.Z },
		{ BoundsMax.X, BoundsMin.Y, BoundsMax.Z },
		{ BoundsMin.X, BoundsMax.Y, BoundsMin.Z },
	};

	TArray<FVector2D, TInlineAllocator<8>> ProjectedBoundsVertices;
	{
		for (const auto& Vertex : BoundsVertices)
		{
			const FVector ProjectedVerted = FVector::VectorPlaneProject(CameraRotation.UnrotateVector(Vertex), FVector::ForwardVector);
			ProjectedBoundsVertices.Add(FVector2D(ProjectedVerted.Y, ProjectedVerted.Z));
		}
	}

	struct FBounds2D { FVector2D Min; FVector2D Max; };
	const auto ProjectedBounds2D = [&]()->FBounds2D
	{
		FVector2D OutMin(EForceInit::ForceInit);
		FVector2D OutMax(EForceInit::ForceInit);
		for (const auto &ProjectedVertex : ProjectedBoundsVertices)
		{
			if (ProjectedVertex.X < OutMin.X)
				OutMin.X = ProjectedVertex.X;
			if (ProjectedVertex.X > OutMax.X)
				OutMax.X = ProjectedVertex.X;

			if (ProjectedVertex.Y < OutMin.Y)
				OutMin.Y = ProjectedVertex.Y;
			if (ProjectedVertex.Y > OutMax.Y)
				OutMax.Y = ProjectedVertex.Y;
		}

		return { OutMin, OutMax };
	}();

	const auto Bounds2DDimentions = FVector2D(FMath::Abs(ProjectedBounds2D.Max.X - ProjectedBounds2D.Min.X), FMath::Abs(ProjectedBounds2D.Max.Y - ProjectedBounds2D.Min.Y));

	// Frame Camera
	const auto CaptureTransform = [&]() 
	{
		const float CameraDistance = [&]() 
		{
			if (ThumbnailSettings.ProjectionType == ECameraProjectionMode::Perspective)
			{
				const float AspectRatio = ThumbnailSettings.ThumbnailTextureWidth > 0 && ThumbnailSettings.ThumbnailTextureHeight > 0 
					? (float)ThumbnailSettings.ThumbnailTextureWidth / (float)ThumbnailSettings.ThumbnailTextureHeight
					: 1.f;

				const float H_HalfFOV = FMath::DegreesToRadians(ThumbnailSettings.CameraFOV * 0.5f);
				const float H_HalfWidth = Bounds2DDimentions.X * 0.5f;

				const float V_HalfFOV = H_HalfFOV / AspectRatio;
				const float V_HalfWidth = Bounds2DDimentions.Y * 0.5f;

				switch (ThumbnailSettings.CameraFitMode)
				{
				case EThumbnailCameraFitMode::EFill:
					return FMath::Min(V_HalfWidth / FMath::Tan(V_HalfFOV), H_HalfWidth / FMath::Tan(H_HalfFOV));
				case EThumbnailCameraFitMode::EFit:
					return FMath::Max(V_HalfWidth / FMath::Tan(V_HalfFOV), H_HalfWidth / FMath::Tan(H_HalfFOV));
				case EThumbnailCameraFitMode::EFitX:
					return H_HalfWidth / FMath::Tan(H_HalfFOV);
				case EThumbnailCameraFitMode::EFitY:
					return V_HalfWidth / FMath::Tan(V_HalfFOV);
				}

				return H_HalfWidth / FMath::Tan(H_HalfFOV);
			}

			// I have observed some strange shader behaviour when the orthographic camera is < 10000cm from the object, not sure why this is as distance is orthographic view should make no difference
			return 10000.0f;
		}();
			
		const auto GetVertexClosestToCameraInCameraSpace = [&]()
		{
			FVector OutVertex(EForceInit::ForceInit);
			for (const auto& Vertex : BoundsVertices)
			{
				const auto RotatedVertex = CameraRotation.UnrotateVector(Vertex);
				if (RotatedVertex.X < OutVertex.X)
					OutVertex = RotatedVertex;
			}

			return OutVertex;
		};

		// Move origin back as to not clip with vertices close to the camera
		const auto OriginInCameraSpace = FVector(GetVertexClosestToCameraInCameraSpace().X, (ProjectedBounds2D.Max.X + ProjectedBounds2D.Min.X) * 0.5f, (ProjectedBounds2D.Max.Y + ProjectedBounds2D.Min.Y) * 0.5f);

		return FTransform(CameraRotation,
			CameraRotation.RotateVector(OriginInCameraSpace)
			+ (-CameraRotation.GetForwardVector() * CameraDistance)
			+ (CameraRotation.GetForwardVector() * ThumbnailSettings.CameraDistanceOffset)
			+ ThumbnailSettings.CameraPositionOffset
		);
	}();

	const float OrthoWidth = [&]()
	{
		if (ThumbnailSettings.ProjectionType == ECameraProjectionMode::Orthographic)
		{
			const float AspectRatio = ThumbnailSettings.ThumbnailTextureWidth > 0 && ThumbnailSettings.ThumbnailTextureHeight > 0
				? (float)ThumbnailSettings.ThumbnailTextureWidth / (float)ThumbnailSettings.ThumbnailTextureHeight
				: 1.f;

			switch (ThumbnailSettings.CameraFitMode)
			{
			case EThumbnailCameraFitMode::EFill:
				return FMath::Min(Bounds2DDimentions.X, Bounds2DDimentions.Y * AspectRatio);
			case EThumbnailCameraFitMode::EFit:
				return FMath::Max(Bounds2DDimentions.X, Bounds2DDimentions.Y * AspectRatio);
			case EThumbnailCameraFitMode::EFitX:
				return Bounds2DDimentions.X;
			case EThumbnailCameraFitMode::EFitY:
				return Bounds2DDimentions.Y * AspectRatio;
			}
		}
		return 0.f;
	}();

	FMinimalViewInfo CaptureComponentView;
	CaptureComponentView.Location       = CaptureTransform.GetLocation();
	CaptureComponentView.Rotation       = CaptureTransform.Rotator();
	CaptureComponentView.ProjectionMode = ThumbnailSettings.ProjectionType;
	CaptureComponentView.FOV            = ThumbnailSettings.CameraFOV;
	CaptureComponentView.OrthoWidth     = OrthoWidth + ThumbnailSettings.OrthoWidthOffset;

	CaptureComponentView.PostProcessBlendWeight = 1.f;
	CaptureComponentView.PostProcessSettings    = ThumbnailSettings.PostProcessingSettings;

	// The Vignette effect is current broken on mobile so make sure to disable it
	#if (PLATFORM_ANDROID || PLATFORM_IOS)
	CaptureComponentView.PostProcessSettings.bOverride_VignetteIntensity = true;
	CaptureComponentView.PostProcessSettings.VignetteIntensity           = 0.f;
	#endif

	CaptureComponent->SetCameraView(CaptureComponentView);
	CaptureComponent->PostProcessSettings    = CaptureComponentView.PostProcessSettings;
	CaptureComponent->PostProcessBlendWeight = CaptureComponentView.PostProcessBlendWeight;
	CaptureComponent->bCameraCutThisFrame    = true; // Reset view each capture
	CaptureComponent->TextureTarget          = RenderTarget;

	TArray<uint8> AlphaOverride;
	if (ThumbnailSettings.bCaptureAlpha)
	{
		// I haven't been able to find a way of extracting the Alpha when capturing SCS_FinalColorHDR/SCS_FinalColorLDR.
		// This is a bit of an ugly hack where we capture the scene again using SCS_SceneColorHDR
		// and copy the alpha results onto our main capture.
		CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR; 
		CaptureComponent->CaptureScene();

		AlphaOverride = Local::ExtractAlpha(RenderTarget, true);

		// Currently on mobile SCS_SceneColorHDR gets flipped Vertically
		if (RHINeedsToSwitchVerticalAxis(GMaxRHIShaderPlatform)) 
		{
			const auto Swap = [](uint8& A, uint8& B) { uint8 Tmp = A; A = B; B = Tmp; };
			for (int32 x = 0; x < RenderTarget->SizeX; x++)
				for (int32 y = 0; y < RenderTarget->SizeY / 2; y++)
					Swap(AlphaOverride[x + (y * RenderTarget->SizeX)], AlphaOverride[x + ((RenderTarget->SizeY - 1 - y) * (RenderTarget->SizeX))]);
		}

		CaptureComponent->CaptureSource = GetCaptureSource();
	}

	CaptureComponent->CaptureScene();

	CaptureComponent->TextureTarget = nullptr;

	UTexture2D* ThumbnailTexture = IsValid(ResourceObject) 
		? ResourceObject
		: Local::ConstructTransientTexture2D(
			UThumbnailGenerator::Get(), 
			FString::Printf(TEXT("%s_Thumbnail"), *Actor->GetName()), 
			RenderTarget->SizeX, 
			RenderTarget->SizeY,
			RenderTarget->GetFormat()
		);

	if (!ThumbnailTexture)
	{
		UE_LOG(LogThumbnailGenerator, Error, TEXT("CaptureThumbnail - Failed to construct Texture2D object"));
		return nullptr;
	}

	Local::FillTextureData(ThumbnailTexture, RenderTarget, AlphaOverride.Num() > 0 ? &AlphaOverride : nullptr, ThumbnailSettings.AlphaBlendMode, CTF_SRGB);

	ThumbnailTexture->SRGB = true;
	ThumbnailTexture->CompressionSettings = TextureCompressionSettings::TC_EditorIcon;
	ThumbnailTexture->LODGroup = TextureGroup::TEXTUREGROUP_UI;

	return ThumbnailTexture;
}

void UThumbnailGenerator::Tick(float DeltaTime)
{
	if (DeferredEventQueue.Num() > 0)
	{
		const auto QueueElement = DeferredEventQueue.Pop();
		QueueElement();
	}
}

void UThumbnailGenerator::BeginDestroy()
{
	Cleanup();
	Super::BeginDestroy();
}

void UThumbnailGenerator::Shutdown()
{
	// Destroy
	RemoveFromRoot();
	MarkPendingKill();
}

void UThumbnailGenerator::Cleanup()
{
	FGameDelegates::Get().GetExitCommandDelegate().RemoveAll(this);
	FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);

	if (ThumbnailScene.IsValid())
		ThumbnailScene.Reset();

	if (ThumbnailCache.IsValid())
		ThumbnailCache->ClearCache();

	if (RenderTargetCache.IsValid())
		RenderTargetCache->ClearCache();

	for (UThumbnailGeneratorScript* ThumbnailGeneratorScript : ThumbnailGeneratorScripts)
		if (IsValid(ThumbnailGeneratorScript))
			ThumbnailGeneratorScript->MarkPendingKill();

	ThumbnailGeneratorScripts.Empty();
}

void UThumbnailGenerator::QueueDeferredTask(const TFunction<void()>& Callback)
{
	DeferredEventQueue.Insert(Callback, 0);
}

/*
* UThumbnailGeneration 
*/

UTexture2D* UThumbnailGeneration::GenerateThumbnail_Synchronous(TSubclassOf<AActor> ActorClass, ECacheMethod CacheMethod, FThumbnailSettings ThumbnailSettings)
{
	if (UThumbnailGenerator* ThumbnailGenerator = UThumbnailGenerator::Get())
	{
		FGenerateThumbnailParams Params;
		Params.CacheMethod       = CacheMethod;
		Params.ThumbnailSettings = FThumbnailSettings::MergeThumbnailSettings(UThumbnailGeneratorSettings::Get()->DefaultThumbnailSettings, ThumbnailSettings);
		return ThumbnailGenerator->GenerateActorThumbnail_Synchronous(ActorClass, Params);
	}

	return nullptr;
}

UWorld* UThumbnailGeneration::GetThumbnailWorld()
{
	if (UThumbnailGenerator* ThumbnailGenerator = UThumbnailGenerator::Get())
		return ThumbnailGenerator->GetWorld();

	return nullptr;
}

USceneCaptureComponent2D* UThumbnailGeneration::GetThumbnailCaptureComponent()
{
	if (UThumbnailGenerator* ThumbnailGenerator = UThumbnailGenerator::Get())
		return ThumbnailGenerator->GetThumbnailCaptureComponent();
	
	return nullptr;
}

void UThumbnailGeneration::InitializeThumbnailWorld(FThumbnailBackgroundSceneSettings BackgroundSceneSettings)
{
	if (UThumbnailGenerator* ThumbnailGenerator = UThumbnailGenerator::Get())
		ThumbnailGenerator->InitializeThumbnailWorld(BackgroundSceneSettings);
}

void UThumbnailGeneration::SaveThumbnail(UTexture2D* Thumbnail, const FDirectoryPath& OutputDirectory, FString OutputName)
{
#if WITH_EDITOR
	FThumbnailGeneratorModule::OnSaveThumbnail.Broadcast(Thumbnail, OutputDirectory.Path, OutputName);
#endif
}

void UThumbnailGeneration::K2_GenerateThumbnail(UClass* ActorClass, ECacheMethod CacheMethod, FThumbnailSettings ThumbnailSettings, 
	TMap<FString, FString> Properties, FGenerateThumbnailCallback Callback, FPreCaptureThumbnail PreCaptureThumbnail)
{
	if (UThumbnailGenerator* ThumbnailGenerator = UThumbnailGenerator::Get())
	{
		FGenerateThumbnailParams Params;
		Params.CacheMethod         = CacheMethod;
		Params.Properties          = Properties;
		Params.ThumbnailSettings   = FThumbnailSettings::MergeThumbnailSettings(UThumbnailGeneratorSettings::Get()->DefaultThumbnailSettings, ThumbnailSettings);
		Params.PreCaptureThumbnail = FPreCaptureActorThumbnail::CreateLambda([PreCaptureThumbnail](AActor* Actor) { PreCaptureThumbnail.ExecuteIfBound(Actor); });

		const auto NativeCallback = FOnThumbnailGenerated::CreateLambda([Callback](UTexture2D* Thumbnail) { Callback.ExecuteIfBound(Thumbnail); });
		ThumbnailGenerator->GenerateActorThumbnail(ActorClass, NativeCallback,Params);
	}
}

DEFINE_FUNCTION(UThumbnailGeneration::execK2_ExportPropertyText)
{
	Stack.StepCompiledIn<FProperty>(NULL);

	auto* Property        = Stack.MostRecentProperty;
	auto* PropertyValAddr = (void*)Stack.MostRecentPropertyAddress;

	P_FINISH;

	P_NATIVE_BEGIN;
	FString PropertyTextValue;
	Property->ExportTextItem(PropertyTextValue, PropertyValAddr, nullptr, nullptr, 0, nullptr);
	*(FString*)RESULT_PARAM = PropertyTextValue;
	P_NATIVE_END;
}

DEFINE_FUNCTION(UThumbnailGeneration::execK2_ExportArrayPropertyText)
{
	Stack.StepCompiledIn<FArrayProperty>(NULL);

	auto* Property        = Stack.MostRecentProperty;
	auto* PropertyValAddr = (void*)Stack.MostRecentPropertyAddress;

	P_FINISH;

	P_NATIVE_BEGIN;
	FString PropertyTextValue;
	Property->ExportTextItem(PropertyTextValue, PropertyValAddr, nullptr, nullptr, 0, nullptr);
	*(FString*)RESULT_PARAM = PropertyTextValue;
	P_NATIVE_END;
}

DEFINE_FUNCTION(UThumbnailGeneration::execK2_ExportMapPropertyText)
{
	Stack.StepCompiledIn<FMapProperty>(NULL);

	auto* Property        = Stack.MostRecentProperty;
	auto* PropertyValAddr = (void*)Stack.MostRecentPropertyAddress;

	P_FINISH;

	P_NATIVE_BEGIN;
	FString PropertyTextValue;
	Property->ExportTextItem(PropertyTextValue, PropertyValAddr, nullptr, nullptr, 0, nullptr);
	*(FString*)RESULT_PARAM = PropertyTextValue;
	P_NATIVE_END;
}

DEFINE_FUNCTION(UThumbnailGeneration::execK2_ExportSetPropertyText)
{
	Stack.StepCompiledIn<FSetProperty>(NULL);

	auto* Property        = Stack.MostRecentProperty;
	auto* PropertyValAddr = (void*)Stack.MostRecentPropertyAddress;

	P_FINISH;

	P_NATIVE_BEGIN;
	FString PropertyTextValue;
	Property->ExportTextItem(PropertyTextValue, PropertyValAddr, nullptr, nullptr, 0, nullptr);
	*(FString*)RESULT_PARAM = PropertyTextValue;
	P_NATIVE_END;
}
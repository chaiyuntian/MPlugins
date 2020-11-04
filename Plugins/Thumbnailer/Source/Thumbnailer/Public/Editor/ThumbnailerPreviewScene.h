/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "AdvancedPreviewScene.h"

class UThumbnailChildActorComponent;

class THUMBNAILER_API FThumbnailerPreviewScene : public FAdvancedPreviewScene
{
public:
	FThumbnailerPreviewScene(ConstructionValues CVS, float InFloorOffset = 0.0f);
	~FThumbnailerPreviewScene();
public:
	void SetChildActorComp(UThumbnailChildActorComponent* comp);

	static UStaticMeshComponent* GetStaticMeshComp() { return MeshComp; }
	static USkeletalMeshComponent* GetSkelMeshComp() { return SkelMeshComp; }
	static UThumbnailChildActorComponent* GetChildActorComp() { return ChildActorComp; }
private:
	static class UStaticMeshComponent* MeshComp;
	static class USkeletalMeshComponent* SkelMeshComp;
	static class UPostProcessComponent* PostComp;
	static class UThumbnailChildActorComponent* ChildActorComp;
private:
	FDelegateHandle SkeletalMeshHandle;
	FDelegateHandle MeshHandle;
	FDelegateHandle ChildActorHandle;

	FDelegateHandle OnSetStaticMeshHandle;
	FDelegateHandle OnSetSkeletalMeshHandle;
	FDelegateHandle OnSetChildActorClassHandle;
private:
	void OnStaticMeshChanged(UStaticMeshComponent* comp);
	void OnSkeletalMeshPropertyChanged();
	void OnChildActorSpawned(TSubclassOf<AActor> actor);

	void OnSetStaticMesh(UStaticMesh* mesh);
	void OnSetSkeletalMesh(USkeletalMesh* mesh);
	void OnSetChildActorClass(TSubclassOf<AActor> actorClass);
};

FORCEINLINE static UStaticMeshComponent* GetStaticMeshComp()
{
	return FThumbnailerPreviewScene::GetStaticMeshComp();
}
FORCEINLINE static USkeletalMeshComponent* GetSkelMeshComp()
{
	return FThumbnailerPreviewScene::GetSkelMeshComp();
}
FORCEINLINE static UThumbnailChildActorComponent* GetChildActorComp()
{
	return FThumbnailerPreviewScene::GetChildActorComp();
}
/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "Editor/ThumbnailerPreviewScene.h"
#include "Components/ThumbnailChildActorComponent.h"
#include "ThumbnailerPluginPrivatePCH.h"
#include "ThumbnailerHelper.h"

UStaticMeshComponent* FThumbnailerPreviewScene::MeshComp = nullptr;
USkeletalMeshComponent* FThumbnailerPreviewScene::SkelMeshComp = nullptr;
UPostProcessComponent* FThumbnailerPreviewScene::PostComp = nullptr;
UThumbnailChildActorComponent* FThumbnailerPreviewScene::ChildActorComp = nullptr;

FThumbnailerPreviewScene::FThumbnailerPreviewScene(ConstructionValues CVS, float InFloorOffset /* = 0.0f */)
	: FAdvancedPreviewScene(CVS, InFloorOffset)
{
	MeshComp = NewObject<UStaticMeshComponent>();
	SkelMeshComp = NewObject<USkeletalMeshComponent>();

	AddComponent(MeshComp, FTransform::Identity, false);
	AddComponent(SkelMeshComp, FTransform::Identity, false);

	MeshComp->bRenderCustomDepth = true;
	MeshComp->SetRenderCustomDepth(true);
	MeshComp->SetVisibility(true);
	MeshComp->bForceMipStreaming = true;
	MeshComp->bIgnoreInstanceForTextureStreaming = true;
	SkelMeshComp->bRenderCustomDepth = true;
	SkelMeshComp->SetRenderCustomDepth(true);
	SkelMeshComp->SetVisibility(true);
	SkelMeshComp->bForceMipStreaming = true;

	if (MeshComp)
	{
		MeshHandle = MeshComp->OnStaticMeshChanged().AddRaw(this, &FThumbnailerPreviewScene::OnStaticMeshChanged);
	}
	if (SkelMeshComp)
	{
		SkeletalMeshHandle = SkelMeshComp->OnSkeletalMeshPropertyChanged.AddRaw(this, &FThumbnailerPreviewScene::OnSkeletalMeshPropertyChanged);
	}

	OnSetStaticMeshHandle = FThumbnailerCore::OnSetStaticMesh.AddRaw(this, &FThumbnailerPreviewScene::OnSetStaticMesh);
	OnSetSkeletalMeshHandle = FThumbnailerCore::OnSetSkeletalMesh.AddRaw(this, &FThumbnailerPreviewScene::OnSetSkeletalMesh);
	OnSetChildActorClassHandle = FThumbnailerCore::OnSetChildActorClass.AddRaw(this, &FThumbnailerPreviewScene::OnSetChildActorClass);
}

FThumbnailerPreviewScene::~FThumbnailerPreviewScene()
{
	MeshComp->OnStaticMeshChanged().Remove(MeshHandle);
	SkelMeshComp->OnSkeletalMeshPropertyChanged.Remove(SkeletalMeshHandle);
	ChildActorComp->OnChildActorSpawned.Remove(ChildActorHandle);

	FThumbnailerCore::OnSetStaticMesh.Remove(OnSetStaticMeshHandle);
	FThumbnailerCore::OnSetSkeletalMesh.Remove(OnSetSkeletalMeshHandle);
	FThumbnailerCore::OnSetChildActorClass.Remove(OnSetChildActorClassHandle);

	MeshHandle.Reset();
	SkeletalMeshHandle.Reset();
	ChildActorHandle.Reset();

	OnSetStaticMeshHandle.Reset();
	OnSetSkeletalMeshHandle.Reset();
	OnSetChildActorClassHandle.Reset();
}

void FThumbnailerPreviewScene::SetChildActorComp(UThumbnailChildActorComponent* comp)
{
	ChildActorComp = comp;

	ChildActorHandle = ChildActorComp->OnChildActorSpawned.AddRaw(this, &FThumbnailerPreviewScene::OnChildActorSpawned);

	AddComponent(ChildActorComp, FTransform::Identity, false);
}

void FThumbnailerPreviewScene::OnStaticMeshChanged(UStaticMeshComponent* comp)
{
	FThumbnailerCore::OnStaticMeshPropertyChanged.Broadcast(comp);
	FThumbnailerCore::SetAssetType(comp->GetStaticMesh() ? EAssetType::StaticMesh : EAssetType::None);
}

void FThumbnailerPreviewScene::OnSkeletalMeshPropertyChanged()
{
	FThumbnailerCore::OnSkeletalMeshPropertyChanged.Broadcast(SkelMeshComp);
	FThumbnailerCore::SetAssetType(SkelMeshComp->SkeletalMesh ? EAssetType::SkeletalMesh : EAssetType::None);
}

void FThumbnailerPreviewScene::OnChildActorSpawned(TSubclassOf<AActor> actor)
{
	FThumbnailerCore::OnChildActorCompPropertyChanged.Broadcast(ChildActorComp);
	FThumbnailerCore::SetAssetType(ChildActorComp->GetChildActorClass() ? EAssetType::Actor : EAssetType::None);
}

void FThumbnailerPreviewScene::OnSetStaticMesh(UStaticMesh* mesh)
{
	if (MeshComp)
	{
		MeshComp->SetStaticMesh(mesh);
		
		if (mesh)
			SetFloorOffset(-mesh->ExtendedBounds.Origin.Z + mesh->ExtendedBounds.BoxExtent.Z);
	}
}

void FThumbnailerPreviewScene::OnSetSkeletalMesh(USkeletalMesh* mesh)
{
	if (SkelMeshComp)
	{
		SkelMeshComp->SetSkeletalMesh(mesh);
		SkelMeshComp->OnSkeletalMeshPropertyChanged.Broadcast();

		if (mesh)
			SetFloorOffset(-mesh->GetBounds().Origin.Z + mesh->GetBounds().BoxExtent.Z);
	}
}

void FThumbnailerPreviewScene::OnSetChildActorClass(TSubclassOf<AActor> actorClass)
{
	if (ChildActorComp != nullptr)
	{
		ChildActorComp->SetChildActorClass(actorClass);

		if (actorClass && ChildActorComp->GetChildActor())
		{
			FVector Origin;
			FVector BoxExtent;
			ChildActorComp->GetChildActor()->GetActorBounds(true, Origin, BoxExtent);

			SetFloorOffset(-Origin.Z + BoxExtent.Z);
		}
	}
}

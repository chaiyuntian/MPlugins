/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "Slate/SThumbnailerWindow.h"
#include "Slate/ThumbnailerViewport.h"
#include "Slate/ThumbnailerCustomization.h"
#include "Actors/ThumbnailActor.h"
#include "Components/ThumbnailChildActorComponent.h"
#include "Editor/ThumbnailerViewportClient.h"
#include "Editor/ThumbnailerPreviewScene.h"
#include "ThumbnailerModule.h"
#include "ThumbnailerHelper.h"

#include "ThumbnailerPluginPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FThumbnailerModule"

void SThumbnailerWindow::Construct(const FArguments& InArgs)
{
	ThumbnailerModulePtr = InArgs._moduleptr;
    ThumbnailActor = InArgs._thumbnailActor;
	ThumbnailViewport = InArgs._thumbnailViewport;
	
	if (ThumbnailerModulePtr == nullptr || ThumbnailActor == nullptr || ThumbnailViewport == nullptr)
	{
		UE_LOG(LogThumbnailer, Error, TEXT("Failed creating thumbnailer window, this should not happen!"));
		return;
	}

	float m_LeftPanel = 0.25f;
	float m_PreviewPanel = 1.0f - m_LeftPanel;

	const FButtonStyle* ButtonStyle = &FThumbnailerStyle::Get().GetWidgetStyle<FButtonStyle>("Thumbnailer.CaptureScreenshotButtonStyle");
	const FButtonStyle* CategoryButtonStyle = &FThumbnailerStyle::Get().GetWidgetStyle<FButtonStyle>("Thumbnailer.CategoryButtonStyle");
	const FButtonStyle* ResetCameraButtonStyle = &FThumbnailerStyle::Get().GetWidgetStyle<FButtonStyle>("Thumbnailer.ResetCameraButtonStyle");
	const FButtonStyle* ResetOptionsButtonStyle = &FThumbnailerStyle::Get().GetWidgetStyle<FButtonStyle>("Thumbnailer.ResetOptionsButtonStyle");

	const FOnGetDetailCustomizationInstance m_StaticMeshOverrideDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FThumbnailerStaticMeshDetails::MakeInstance);
	const FOnGetDetailCustomizationInstance m_SkeletalMeshOverrideDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FThumbnailerSkeletalMeshDetails::MakeInstance);
	const FOnGetDetailCustomizationInstance m_ActorOverrideDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FThumbnailerActorDetails::MakeInstance);

	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		FDetailsViewArgs m_DetailArgs(false, false, false, FDetailsViewArgs::HideNameArea, false);
		m_DetailArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
		m_DetailArgs.bShowOptions = false;
	
		MeshView = PropertyModule.CreateDetailView(m_DetailArgs);
		SkelMeshView = PropertyModule.CreateDetailView(m_DetailArgs);
		ChildActorComp = PropertyModule.CreateDetailView(m_DetailArgs);
		MeshView->RegisterInstancedCustomPropertyLayout(UStaticMeshComponent::StaticClass(), m_StaticMeshOverrideDetails);
		SkelMeshView->RegisterInstancedCustomPropertyLayout(USkeletalMeshComponent::StaticClass(), m_SkeletalMeshOverrideDetails);
		ChildActorComp->RegisterInstancedCustomPropertyLayout(UChildActorComponent::StaticClass(), m_ActorOverrideDetails);
		SkelMeshView->SetObject(FThumbnailerPreviewScene::GetSkelMeshComp(), true);
		MeshView->SetObject(FThumbnailerPreviewScene::GetStaticMeshComp(), true);
		ChildActorComp->SetObject(ThumbnailActor->GetChildActorComp(), true);
	}

	ChildSlot
		[
            SNew(SBorder)
				[
					SNew(SSplitter).PhysicalSplitterHandleSize(5.f).Orientation(Orient_Horizontal)
				+ SSplitter::Slot().Value(m_LeftPanel)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
							[
								SNew(SBox).Padding(4.0f)
							[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBorder).BorderImage(FEditorStyle::GetBrush("Scalability.RowBackground")).Padding(5.f).BorderBackgroundColor(FColor::Black)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().HAlign(HAlign_Fill)
										[
											SNew(STextBlock).Text(FText::FromString("DISPLAY SETTINGS")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
										]
									+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right).VAlign(VAlign_Center)
										[
											SNew(SButton).ButtonStyle(ResetCameraButtonStyle).OnClicked_Raw(this, &SThumbnailerWindow::HandleCollapseButtonClicked).ToolTipText(LOCTEXT("CollapseText", "Collpase Categories"))
											[
												SNew(STextBlock).Text(FText::FromString("-")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 8))
											]
										]
								]
							]
						+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SButton).ButtonStyle(CategoryButtonStyle).OnClicked_Lambda([this](){StaticMeshSwitcher->SetActiveWidgetIndex(StaticMeshSwitcher->GetActiveWidgetIndex() == 0 ? 1 : 0); return FReply::Handled();})
									[
										SNew(STextBlock).Text(FText::FromString("STATIC MESH")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10))
									]
							]
						+ SVerticalBox::Slot().AutoHeight()
							[
								SAssignNew(StaticMeshSwitcher, SWidgetSwitcher).WidgetIndex(0)
								+ SWidgetSwitcher::Slot()
									[
										SNew(STextBlock).Text(FText::FromString(" ")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 1))
									]
								+ SWidgetSwitcher::Slot()
									[
										MeshView->AsShared()
									]
							]			
						+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SButton).ButtonStyle(CategoryButtonStyle).OnClicked_Lambda([this](){SkeletalMeshSwitcher->SetActiveWidgetIndex(SkeletalMeshSwitcher->GetActiveWidgetIndex() == 0 ? 1 : 0); return FReply::Handled();})
								[
									SNew(STextBlock).Text(FText::FromString("SKELETAL MESH")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10))
								]
							]
						+ SVerticalBox::Slot().AutoHeight()
							[
								SAssignNew(SkeletalMeshSwitcher, SWidgetSwitcher).WidgetIndex(0)
								+ SWidgetSwitcher::Slot()
									[
										SNew(STextBlock).Text(FText::FromString(" ")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 1))
									]
								+ SWidgetSwitcher::Slot()
									[
										SkelMeshView->AsShared()
									]
							]
						+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SButton).ButtonStyle(CategoryButtonStyle).OnClicked_Lambda([this](){ChildActorSwitcher->SetActiveWidgetIndex(ChildActorSwitcher->GetActiveWidgetIndex() == 0 ? 1 : 0); return FReply::Handled();})
								[
									SNew(STextBlock).Text(FText::FromString("ACTOR")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10))
								]
							]
						+ SVerticalBox::Slot().AutoHeight()
							[
								SAssignNew(ChildActorSwitcher, SWidgetSwitcher).WidgetIndex(0)
								+ SWidgetSwitcher::Slot()
									[
										SNew(STextBlock).Text(FText::FromString(" ")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 1))
									]
								+ SWidgetSwitcher::Slot()
									[
										ChildActorComp->AsShared()
									]
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
					[
						SNew(SButton).ButtonStyle(ResetOptionsButtonStyle).OnClicked_Lambda([this]() { FThumbnailerCore::OnResetMeshSelectionsButtonClicked.Broadcast(); return FReply::Handled(); }).ToolTipText(LOCTEXT("ResetSettingsText", "Reset Mesh Selection Settings"))
							[
								SNew(STextBlock).Text(FText::FromString("RESET")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 8))
							]
					]
				]
			+ SSplitter::Slot().Value(m_PreviewPanel)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
						[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Left)
							[
								SNew(SButton).ButtonStyle(ButtonStyle).OnClicked_Lambda([this]() { FThumbnailerCore::OnCaptureThumbnailButtonClicked.Broadcast(); return FReply::Handled(); })
								[
									SNew(STextBlock).Text(FText::FromString("CAPTURE THUMBNAIL")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
								]
							]

				+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Right).AutoWidth()
					[
						SNew(SButton).ButtonStyle(ResetCameraButtonStyle).OnClicked_Lambda([this]() { FThumbnailerCore::OnResetCameraButtonClicked.Broadcast(); return FReply::Handled(); })
						[
							SNew(STextBlock).Text(FText::FromString("RESET CAMERA")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 8)).ToolTipText(LOCTEXT("ResetCameraButtonText", "Reset camera to default view"))
						]
					]
				+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Right).AutoWidth()
					[
						SNew(SButton).ButtonStyle(ResetCameraButtonStyle).OnClicked_Lambda([this](){ FThumbnailerCore::OnFocusCameraButtonClicked.Broadcast(); return FReply::Handled(); })
						[
							SNew(STextBlock).Text(FText::FromString("FOCUS CAMERA")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 8)).ToolTipText(LOCTEXT("FocusCameraButtonText", "Focus camera on the current asset"))
						]
					]
				+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Right).AutoWidth()
					[
						SNew(SButton).ButtonStyle(ResetCameraButtonStyle).OnClicked_Lambda([this]() { FThumbnailerCore::OnSaveCameraPositionButtonClicked.Broadcast(FThumbnailerCameraSettings(ThumbnailViewport->GetViewportClient()->GetViewLocation(), ThumbnailViewport->GetViewportClient()->GetViewRotation())); return FReply::Handled(); })
						[
							SNew(STextBlock).Text(FText::FromString("SAVE CAMERA")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 8)).ToolTipText(LOCTEXT("SaveCameraButtonText", "Save startup camera location / rotation"))
						]
					]
				+ SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Right).AutoWidth()
					[
						SNew(SButton).ButtonStyle(ResetCameraButtonStyle).OnClicked_Lambda([this]()
							{
								FGlobalTabmanager::Get()->InvokeTab(FName("ThumbnailerSettings"));
								return FReply::Handled();
							})
							[
								SNew(STextBlock).Text(FText::FromString("SETTINGS")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 8)).ToolTipText(LOCTEXT("SettingsButtonText", "Open Thumbnailer Settings"))
							]
					]
				]

			+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SSeparator).Orientation(Orient_Horizontal)
				]

			+ SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().MaxWidth(3.0f)
						[
							SNew(SSeparator)
							.Orientation(Orient_Horizontal)
						]
					+ SHorizontalBox::Slot().FillWidth(100.0f)
						[
							ThumbnailViewport->AsShared()
						]
				]
			]
		]
	]
	];

	OnStaticMeshPropertyChangedHandle = FThumbnailerCore::OnStaticMeshPropertyChanged.AddRaw(this, &SThumbnailerWindow::OnStaticMeshPropertyChanged);
	OnSkeletalMeshPropertyChangedHandle = FThumbnailerCore::OnSkeletalMeshPropertyChanged.AddRaw(this, &SThumbnailerWindow::OnSkeletalMeshPropertyChanged);
	OnChildActorCompPropertyChangedHandle = FThumbnailerCore::OnChildActorCompPropertyChanged.AddRaw(this, &SThumbnailerWindow::OnChildActorCompPropertyChanged);

	OnCollpaseCategoriesHandle = FThumbnailerCore::OnCollpaseCategories.AddLambda([this]() { HandleCollapseButtonClicked(); });
}

SThumbnailerWindow::~SThumbnailerWindow()
{
	FThumbnailerCore::OnStaticMeshPropertyChanged.Remove(OnStaticMeshPropertyChangedHandle);
	FThumbnailerCore::OnSkeletalMeshPropertyChanged.Remove(OnSkeletalMeshPropertyChangedHandle);
	FThumbnailerCore::OnChildActorCompPropertyChanged.Remove(OnChildActorCompPropertyChangedHandle);
	FThumbnailerCore::OnCollpaseCategories.Remove(OnCollpaseCategoriesHandle);
}

void SThumbnailerWindow::OnStaticMeshPropertyChanged(UStaticMeshComponent* comp)
{
	if (StaticMeshSwitcher)
		StaticMeshSwitcher->SetActiveWidgetIndex(comp->GetStaticMesh() ? 1 : 0);
}

void SThumbnailerWindow::OnSkeletalMeshPropertyChanged(USkeletalMeshComponent* comp)
{
	if (SkeletalMeshSwitcher)
		SkeletalMeshSwitcher->SetActiveWidgetIndex(comp->SkeletalMesh ? 1 : 0);
}

void SThumbnailerWindow::OnChildActorCompPropertyChanged(UThumbnailChildActorComponent* comp)
{
	if (ChildActorSwitcher)
		ChildActorSwitcher->SetActiveWidgetIndex(comp->GetChildActorClass() ? 1 : 0);
}

FReply SThumbnailerWindow::HandleCollapseButtonClicked()
{
	if (StaticMeshSwitcher && SkeletalMeshSwitcher && ChildActorSwitcher)
	{
		StaticMeshSwitcher->SetActiveWidgetIndex(0);
		SkeletalMeshSwitcher->SetActiveWidgetIndex(0);
		ChildActorSwitcher->SetActiveWidgetIndex(0);
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
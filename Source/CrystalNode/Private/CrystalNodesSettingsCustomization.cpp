#include "CrystalNodesSettingsCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Settings/EditorStyleSettings.h"
#include "Slate/SlateBrushAsset.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

TSharedRef<IDetailCustomization> FCrystalNodesSettingsCustomization::MakeInstance()
{
	return MakeShareable(new FCrystalNodesSettingsCustomization);
}

void FCrystalNodesSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Setup");

	Category.AddCustomRow(NSLOCTEXT("CrystalNodes", "SetupBGText", "Background"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("CrystalNodes", "SetupBGBtn", "Set up graph background"))
				.ToolTipText(NSLOCTEXT("CrystalNodes", "SetupBGTooltip", "Set the background of Crystal Nodes"))
				.OnClicked(this, &FCrystalNodesSettingsCustomization::OnSetupClicked)
				.HAlign(HAlign_Left)
			]
			
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("CrystalNodes", "ResetBGBtn", "Reset graph background"))
				.ToolTipText(NSLOCTEXT("CrystalNodes", "ResetBGTooltip", "Reset the graph background to default"))
				.OnClicked(this, &FCrystalNodesSettingsCustomization::OnResetClicked)
				.HAlign(HAlign_Left)
			]
		];
}

void SendSetupMessage(bool Success, FText Message)
{
	FNotificationInfo Info(Message);
	Info.Image = Success ? Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.SuccessImage")) : Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.FailImage"));
	Info.ExpireDuration = 3.0f;
	Info.bUseSuccessFailIcons = true;
	Info.bFireAndForget = true;
	FSlateNotificationManager::Get().AddNotification(Info);
}

FReply FCrystalNodesSettingsCustomization::OnSetupClicked()
{
	UEditorStyleSettings* EditorSettings = GetMutableDefault<UEditorStyleSettings>();
    
	if (!EditorSettings)
	{
		SendSetupMessage(false, NSLOCTEXT("CrystalNodes", "SetupFail", "Set up failed."));
		return FReply::Handled();
	}
	USlateBrushAsset* GraphBG = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_GraphBackground.SB_GraphBackground'"));
	if (!GraphBG)
	{
		SendSetupMessage(false, NSLOCTEXT("CrystalNodes", "SetupFail_NoAsset", "Set up failed, no valid background asset."));
		return FReply::Handled();
	}

	EditorSettings->bUseGrid = false;
	EditorSettings->GraphBackgroundBrush = GraphBG->Brush;
	EditorSettings->SaveConfig();
	SendSetupMessage(true, NSLOCTEXT("CrystalNodes", "SetupSucceed", "Graph Background is set up successfully"));
	return FReply::Handled();
}

FReply FCrystalNodesSettingsCustomization::OnResetClicked()
{
	UEditorStyleSettings* EditorSettings = GetMutableDefault<UEditorStyleSettings>();
    
	if (!EditorSettings)
	{
		SendSetupMessage(false, NSLOCTEXT("CrystalNodes", "ResetFail", "Reset failed."));
		return FReply::Handled();
	}
	EditorSettings->bUseGrid = true;
	EditorSettings->GraphBackgroundBrush = FSlateBrush();
	EditorSettings->SaveConfig();
	SendSetupMessage(true, NSLOCTEXT("CrystalNodes", "ResetSucceed", "Graph Background is now back to default"));
	return FReply::Handled();
}

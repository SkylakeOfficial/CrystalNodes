// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrystalNodes.h"

#include "MaterialDomain.h"
#include "SGraphPanel.h"
#include "ToolMenus.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "MaterialGraph/MaterialGraph.h"
#include "Styling/SlateStyle.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#include "Materials/MaterialParameterCollection.h"
#include "Slate/SlateBrushAsset.h"
#include "Styling/SlateStyleMacros.h"

#define LOCTEXT_NAMESPACE "FCrystalNodesModule"

bool FCrystalInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	bIsDragging = MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);
	return false;
}
 
bool FCrystalInputProcessor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	bIsDragging = false;
	return false;
}

void FCrystalNodesModule::StartupModule()
{
	MyProcessor = new FCrystalInputProcessor();
	BeautifyEditor();
	TickDelegate = FTickerDelegate::CreateRaw( this, &FCrystalNodesModule::Tick );
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker( TickDelegate );
	EditorMatParams = LoadObject<UMaterialParameterCollection>(nullptr,TEXT("/Script/Engine.MaterialParameterCollection'/CrystalNodes/Materials/Utils/MP_CrystalEditorParams.MP_CrystalEditorParams'"));
}

void FCrystalNodesModule::ShutdownModule()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(MakeShareable(MyProcessor));
		FSlateApplication::Get().OnFocusChanging().Remove(SlateHandle);
	}
}

bool FCrystalNodesModule::Tick(float DeltaTime)
{
if (!GIsPlayInEditorWorld)//No need if running in PIE
{
	if ( FSlateApplication::IsInitialized())
	{
		if (!bEverBoundSlateDelegates)
		{
			BindSlateDelegates();
		}
		if (FocusingGraph && GraphSelectionAnimVal <= 1.0)
		{
			GraphSelectionAnimVal += DeltaTime;
			UUnrealEditorSubsystem* EditorSys = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
			UKismetMaterialLibrary::SetScalarParameterValue(EditorSys->GetEditorWorld(), EditorMatParams,"GraphSelectionAnimVal",GraphSelectionAnimVal);
		}
		
		if (auto Window =  FSlateApplication::Get().GetActiveTopLevelRegularWindow())
		{
			FVector2D MousePosition = FSlateApplication::Get().GetCursorPos();
			FVector2D MouseLastPosition = FSlateApplication::Get().GetLastCursorPos();
			FVector2D WindowSize = Window->GetSizeInScreen();
			FVector2D WindowPos = FSlateApplication::Get().GetActiveTopLevelWindow()->GetPositionInScreen();
			//Normalize Cursor Positions
			MousePosition.X = UKismetMathLibrary::MapRangeClamped(MousePosition.X,WindowPos.X,WindowPos.X+WindowSize.X,0.0,1.0);
			MousePosition.Y = UKismetMathLibrary::MapRangeClamped(MousePosition.Y,WindowPos.Y,WindowPos.Y+WindowSize.Y,0.0,1.0);
			MouseLastPosition.X = UKismetMathLibrary::MapRangeClamped(MouseLastPosition.X,WindowPos.X,WindowPos.X+WindowSize.X,0.0,1.0);
			MouseLastPosition.Y = UKismetMathLibrary::MapRangeClamped(MouseLastPosition.Y,WindowPos.Y,WindowPos.Y+WindowSize.Y,0.0,1.0);
		
			CursorVelTarget = (MousePosition - MouseLastPosition)*DeltaTime;
			CursorVel = FMath::Vector2DInterpConstantTo(CursorVel,CursorVelTarget,DeltaTime,10.0f);
			
			if (MyProcessor->bIsDragging && FocusingGraph)
			{
				CursorDragOffset += CursorVelTarget;
				IsDraggingVal = FMath::Clamp(IsDraggingVal + DeltaTime*2.5f,0.0f,1.0f);
			}
			else
			{
				IsDraggingVal = FMath::Clamp(IsDraggingVal - DeltaTime*2.5f,0.0f,1.0f);				
			}
			
			UUnrealEditorSubsystem* EditorSys = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
			if (EditorMatParams && EditorSys)
			{
				//Write Material Parameters
				UKismetMaterialLibrary::SetScalarParameterValue(EditorSys->GetEditorWorld(), EditorMatParams,"CursorX",MousePosition.X);
				UKismetMaterialLibrary::SetScalarParameterValue(EditorSys->GetEditorWorld(), EditorMatParams,"CursorY",MousePosition.Y);
				UKismetMaterialLibrary::SetScalarParameterValue(EditorSys->GetEditorWorld(), EditorMatParams,"ViewportRatio",WindowSize.X/WindowSize.Y);
				UKismetMaterialLibrary::SetScalarParameterValue(EditorSys->GetEditorWorld(), EditorMatParams,"IsDragging",IsDraggingVal);
				UKismetMaterialLibrary::SetVectorParameterValue(EditorSys->GetEditorWorld(), EditorMatParams,"DragOffsetAndCursorVel",
					FLinearColor(CursorDragOffset.X, CursorDragOffset.Y,CursorVel.X,CursorVel.Y));
			}
		}
	}
}
	return true;
}

#define RootToContentDir Style-> RootToContentDir

void FCrystalNodesModule::BeautifyEditor()
{

	FSlateStyleSet* Style = (FSlateStyleSet*)&FAppStyle::Get();
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("CrystalNodes")->GetBaseDir() / TEXT("Resources/IMG"));
	
	//TODO: Style->Set("Graph.PlayInEditor", new BOX_BRUSH());
	Style->Set("Graph.Node.ShadowSize", FVector2D(32, 32));
	//Regular node styles
	FSlateBrush RegularBody = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_RegularBody.SB_RegularBody'"))->Brush;
	FSlateBrush RegularColor = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_RegularColor.SB_RegularColor'"))->Brush;
	FSlateBrush RegularSelection = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_RegularSelection.SB_RegularSelection'"))->Brush;
	Style->Set("Graph.Node.Body", new FSlateBrush(RegularBody) );
	Style->Set("Graph.Node.TintedBody", new BOX_BRUSH("Blank", FMargin(0.25f)));
	Style->Set("Graph.Node.ColorSpill",new FSlateBrush(RegularColor));
	Style->Set("Graph.Node.TitleHighlight", new BOX_BRUSH("Blank", FMargin(0.25f)));
	Style->Set("Graph.Node.TitleGloss", new BOX_BRUSH("Blank", FMargin(0.25f)));
	Style->Set("Graph.Node.ShadowSelected",new FSlateBrush(RegularSelection));
	Style->Set("Graph.Node.Shadow", new BOX_BRUSH("RegularNode_shadow", FMargin(0.48f, 0.48f)));
	//Var node styles
	FSlateBrush VarBody = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_VarBody.SB_VarBody'"))->Brush;
	FSlateBrush VarColor = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_VarColor.SB_VarColor'"))->Brush;
	FSlateBrush VarSelection = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_VarSelection.SB_VarSelection'"))->Brush;
	Style->Set("Graph.VarNode.Body", new FSlateBrush(VarBody));
	Style->Set("Graph.VarNode.ColorSpill", new FSlateBrush(VarColor));
	Style->Set("Graph.VarNode.Gloss", new BOX_BRUSH("Blank", FMargin(0.25)));
	Style->Set("Graph.VarNode.ShadowSelected", new FSlateBrush(VarSelection));
	Style->Set("Graph.VarNode.Shadow", new BOX_BRUSH("VarNode_shadow", FMargin(0.35f, 0.25f)));

	Style->Set("Graph.CollapsedNode.Body", new BOX_BRUSH("Blank", FMargin(16.f / 64.f, 25.f / 64.f, 16.f / 64.f, 16.f / 64.f)));
	Style->Set("Graph.CollapsedNode.BodyColorSpill", new BOX_BRUSH("Blank", FMargin(16.f / 64.f, 25.f / 64.f, 16.f / 64.f, 16.f / 64.f)));
	//Pin styles
	Style->Set("Graph.Pin.Connected_VarA", new IMAGE_BRUSH("Pin_Connected", FVector2D(10.66f)));
	Style->Set("Graph.Pin.Disconnected_VarA", new IMAGE_BRUSH("Pin_Disconnected", FVector2D(10.66f)));
	Style->Set("Graph.Pin.Connected", new IMAGE_BRUSH("Pin_Connected", FVector2D(10.66f)));
	Style->Set("Graph.Pin.Disconnected", new IMAGE_BRUSH("Pin_Disconnected", FVector2D(10.66f)));
	Style->Set("Graph.ExecPin.Connected", new IMAGE_BRUSH("ExecPin_Connected", FVector2D(14.66f)));
	Style->Set("Graph.ExecPin.Disconnected", new IMAGE_BRUSH("ExecPin_Disconnected", FVector2D(14.66f)));
	Style->Set("Graph.ExecPin.ConnectedHovered", new IMAGE_BRUSH("ExecPin_ConnectedHovered", FVector2D(14.66f)));
	Style->Set("Graph.ExecPin.DisconnectedHovered", new IMAGE_BRUSH("ExecPin_DisconnectedHovered", FVector2D(14.66f)));
	Style->Set("Graph.RefPin.Connected", new IMAGE_BRUSH("RefPin_Connected", FVector2D(10.66f)));
	Style->Set("Graph.RefPin.Disconnected", new IMAGE_BRUSH("RefPin_Disconnected", FVector2D(10.66f)));
	Style->Set("Graph.Pin.BackgroundHovered", new BOX_BRUSH("Pin.BackgroundHovered", FMargin(0.5f,0.4f,0.0f,0.4f)));
	//Node icons
	Style->Set("Graph.Latent.LatentIcon", new IMAGE_BRUSH("Latent.LatentIcon", FVector2D(72.0f*0.66f)));
	Style->Set("Graph.Message.MessageIcon", new IMAGE_BRUSH("Message.MessageIcon", FVector2D(72.0f*0.66f)));
	Style->Set("Graph.Event.InterfaceEventIcon", new IMAGE_BRUSH("Event.InterfaceEventIcon", FVector2D(72.0f*0.66f)));
	Style->Set("Graph.Event.InterfaceEventIcon", new IMAGE_BRUSH("Event.InterfaceEventIcon", FVector2D(72.0f*0.66f)));
	Style->Set("Graph.Replication.ClientEvent", new IMAGE_BRUSH("Replication.ClientEvent", FVector2D(72.0f*0.66f)));
	Style->Set("Graph.Replication.AuthorityOnly", new IMAGE_BRUSH("Replication.AuthorityOnly", FVector2D(72.0f*0.66f)));
	
	Style->Set("KismetExpression.ReadVariable.Body", new BOX_BRUSH("Pin.BackgroundHovered", FMargin(0.5f,0.4f,0.0f,0.4f)));
	Style->Set("KismetExpression.ReadAutogeneratedVariable.Body", new BOX_BRUSH("Pin.BackgroundHovered", FMargin(0.5f,0.4f,0.0f,0.4f)));
	
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}
#undef RootToContentDir


enum EMatSupported
{
	Supported,
	NotSupported,
	NoChange
};
EMatSupported IsMatOfGraphSupported(TSharedPtr<SGraphPanel> Graph)
{
	if(UMaterialGraph* MaterialGraph = Cast<UMaterialGraph>(Graph->GetGraphObj()))
	{
		if (!MaterialGraph->MaterialFunction)
		{
			if (MaterialGraph->Material->MaterialDomain==EMaterialDomain::MD_UI)
			{
				return Supported;
			}
		}
		return NotSupported;
	}
	return NoChange;	
}

void FCrystalNodesModule::OnSlateFocusChange(const FFocusEvent& FocusEvent, const FWeakWidgetPath& WeakWidgetPath, const TSharedPtr<SWidget>& Widget, const FWidgetPath& WidgetPath, const TSharedPtr<SWidget>& Shared)
{
	if (Shared)
	{
		if (Shared->GetType() == FName("SGraphPanel"))
		{
			if (TSharedPtr<SGraphPanel> Graph =  StaticCastSharedPtr<SGraphPanel>(Shared))
			{
				EMatSupported Supported = IsMatOfGraphSupported(Graph);
				if (Supported!=NoChange)
				{
					UUnrealEditorSubsystem* EditorSys = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
					UKismetMaterialLibrary::SetScalarParameterValue(EditorSys->GetEditorWorld(), EditorMatParams,"IsSupported",Supported==EMatSupported::Supported? 1.0 : 0.0);
				}
				if (Graph->GetParentWidget()->GetParentWidget()->GetParentWidget()->GetParentWidget()->GetParentWidget()->GetType() == FName("SBorder"))
				{	
					FocusingGraph = Graph.Get();
					GraphSelectionAnimVal = 0.0;
					if(TSharedPtr<STextBlock> GraphText = StaticCastSharedRef<STextBlock>(Graph->GetParentWidget()->GetAllChildren()->GetChildAt(3)))
					{
						//FText Text =FText::AsCultureInvariant(TEXT("ArklapseFP: ")/GraphText->GetText().ToString() );
						FText Text =FText::AsCultureInvariant(TEXT(""));
						GraphText->SetText(Text);
					}
					return;
				}
			}
		}
	}
	if (Widget)
	{
		if (Widget->GetType() == FName("SGraphPanel"))
		{
			UUnrealEditorSubsystem* EditorSys = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
			UKismetMaterialLibrary::SetScalarParameterValue(EditorSys->GetEditorWorld(), EditorMatParams,"GraphSelectionAnimVal",0.0);
		}
	}
	FocusingGraph = nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCrystalNodesModule, CrystalNode)
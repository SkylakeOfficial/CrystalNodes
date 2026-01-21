// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrystalNodes.h"

#include "CrystalNodesSettings.h"
#include "CrystalNodesSettingsCustomization.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "SGraphPanel.h"
#include "ToolMenus.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#include "Slate/SlateBrushAsset.h"
#include "Styling/SlateStyleMacros.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderResource.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "RenderingThread.h"

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

TArray<FFloat16Color> FCNUniformBuffer::ParamData;
bool FCNUniformBuffer::bEverInitialized;
UTextureRenderTarget2D* FCNUniformBuffer::RenderTarget;

void FCNUniformBuffer::initialize()
{
	RenderTarget = LoadObject<UTextureRenderTarget2D>(nullptr,TEXT("/Script/Engine.TextureRenderTarget2D'/CrystalNodes/Materials/Utils/RT_CrystalNodesUniformBuffer.RT_CrystalNodesUniformBuffer'"));
	ParamData.Init(FFloat16Color(), BufferSize);
	bEverInitialized = true;
}

void FCNUniformBuffer::Set(const FName ParamName, const FFloat16& Param)
{
	if (!bEverInitialized)
	{
		initialize();
	}
	if (!GetParamDefinition().Contains(ParamName))
	{
		return;
	}
	const FUintPoint& Definition = GetParamDefinition()[ParamName];
	if (Definition.Y & 1u)
	{
		ParamData[Definition.X].A = Param;
	}
	if (Definition.Y & 1u << 1)
	{
		ParamData[Definition.X].B = Param;
	}
	if (Definition.Y & 1u << 2)
	{
		ParamData[Definition.X].G = Param;
	}
	if (Definition.Y & 1u << 3)
	{
		ParamData[Definition.X].R = Param;
	}
}

void FCNUniformBuffer::Set(const FName ParamName, const FFloat16Color& Param)
{
	if (!bEverInitialized)
	{
		initialize();
	}
	if (!GetParamDefinition().Contains(ParamName))
	{
		return;
	}
	const FUintPoint& Definition = GetParamDefinition()[ParamName];
	if (Definition.Y & 1u)
	{
		ParamData[Definition.X].A = Param.A;
	}
	if (Definition.Y & 1u << 1)
	{
		ParamData[Definition.X].B = Param.B;
	}
	if (Definition.Y & 1u << 2)
	{
		ParamData[Definition.X].G = Param.G;
	}
	if (Definition.Y & 1u << 3)
	{
		ParamData[Definition.X].R = Param.R;
	}
}

void FCNUniformBuffer::Update()
{
	if (!bEverInitialized)
	{
		initialize();
	}
	if (!RenderTarget)
	{
		return;
	}
	FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
	uint8* Data = (uint8*)ParamData.GetData();
	int32 DataSize = BufferSize;

	ENQUEUE_RENDER_COMMAND(WriteDataToRT)(
		[RTResource, Data, DataSize](FRHICommandListImmediate& RHICmdList)
		{
			FRHITexture* RHITexture = RTResource->GetRenderTargetTexture();
			RHICmdList.Transition(FRHITransitionInfo(RHITexture, ERHIAccess::Unknown, ERHIAccess::RTV));
			uint32 Stride = 0;
			void* TextureData = RHICmdList.LockTexture2D(
				RHITexture,
				0,
				EResourceLockMode::RLM_WriteOnly,
				Stride,
				false
			);
			for (int32 Y = 0; Y < 1; ++Y)
			{
				FMemory::Memcpy(
					(uint8*)TextureData + Y * Stride,
					&Data[Y * DataSize],
					DataSize * sizeof(FFloat16Color)
				);
			}
			RHICmdList.UnlockTexture2D(RHITexture, 0, false);
		}
	);
}

void FCrystalNodesModule::OnSettingChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	UpdateUserSettings();
}

void FCrystalNodesModule::UpdateUserSettings()
{
	const UCrystalNodesSettings* Settings = GetDefault<UCrystalNodesSettings>();
	FCNUniformBuffer::Set("UserScale", Settings->GridScale);
	FCNUniformBuffer::Set("UserThemeColor", Settings->ThemeColor);
	FCNUniformBuffer::Set("UserLightColor", Settings->LightColor);
	FCNUniformBuffer::Set("UserEnablePhong", Settings->PhongLight);
	FCNUniformBuffer::Set("UserEnableCursorDF", Settings->CursorDistanceField);
	FCNUniformBuffer::Set("UserNodeOutline", Settings->NodeOutlineWidth);
	FCNUniformBuffer::Set("UserVarNodeOutline", Settings->VarNodeOutlineWidth);
	FCNUniformBuffer::Set("UserNodeExpand", Settings->NodeOutlineExpansion);
	FCNUniformBuffer::Set("UserVarNodeExpand", Settings->VarNodeOutlineExpansion);
	FCNUniformBuffer::Set("UserBlueNoiseAmount", Settings->BlueNoiseAmount);
}

void FCrystalNodesModule::StartupModule()
{
	CNInputProcessor = new FCrystalInputProcessor();
	BeautifyEditor();
	TickDelegate = FTickerDelegate::CreateRaw(this, &FCrystalNodesModule::Tick);
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);
	
	UCrystalNodesSettings* Settings = GetMutableDefault<UCrystalNodesSettings>();
	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");
	SettingChangeDelegate = Settings->OnSettingChanged().AddRaw(this, &FCrystalNodesModule::OnSettingChanged);
	
SettingsModule.RegisterSettings(
		"Editor",
		"Plugins",
		"CrystalNodes",
		NSLOCTEXT("CrystalNodes", "Name", "Crystal Nodes"),
		NSLOCTEXT("CrystalNodes", "Desc", "Global editor settings for Crystal Nodes"),
		Settings
	);

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		UCrystalNodesSettings::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FCrystalNodesSettingsCustomization::MakeInstance)
	);
}

void FCrystalNodesModule::ShutdownModule()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(MakeShareable(CNInputProcessor));
		FSlateApplication::Get().OnFocusChanging().Remove(SlateHandle);
	}
	
	if (FModuleManager::Get().IsModuleLoaded("Settings"))
	{
		ISettingsModule& SettingsModule = FModuleManager::GetModuleChecked<ISettingsModule>("Settings");
		SettingsModule.UnregisterSettings(
			"Editor",
			"Plugins",
			"CrystalNodes"
		);
	}
	
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(UCrystalNodesSettings::StaticClass()->GetFName());
	}
}


bool FCrystalNodesModule::Tick(float DeltaTime)
{
	if (!GEditor->bIsSimulatingInEditor && !GEditor->PlayWorld) //No need if running in PIE
	{
		if (FSlateApplication::IsInitialized())
		{
			auto Settings = GetDefault<UCrystalNodesSettings>();
			if (!bEverBoundSlateDelegates)
			{
				BindSlateDelegates();
			}
			if (FocusedGraph && GraphSelectionAnimValue <= 1.0)
			{
				GraphSelectionAnimValue += DeltaTime;
				UUnrealEditorSubsystem* EditorSys = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
				FCNUniformBuffer::Set("FocusAnimation", Settings->AnimateBackground ? GraphSelectionAnimValue : 0);
			}

			if (auto Window = FSlateApplication::Get().GetActiveTopLevelRegularWindow())
			{
				FVector2D MousePosition = FSlateApplication::Get().GetCursorPos();
				FVector2D MouseLastPosition = FSlateApplication::Get().GetLastCursorPos();
				FVector2D WindowSize = Window->GetSizeInScreen();
				FVector2D WindowPos = Window->GetPositionInScreen();

				MousePosition.X = FMath::GetMappedRangeValueClamped(FVector2D(WindowPos.X, WindowPos.X + WindowSize.X), FVector2D(0.0, 1.0), MousePosition.X);
				MousePosition.Y = FMath::GetMappedRangeValueClamped(FVector2D(WindowPos.Y, WindowPos.Y + WindowSize.Y), FVector2D(0.0, 1.0), MousePosition.Y);
				MouseLastPosition.X = FMath::GetMappedRangeValueClamped(FVector2D(WindowPos.X, WindowPos.X + WindowSize.X), FVector2D(0.0, 1.0), MouseLastPosition.X);
				MouseLastPosition.Y = FMath::GetMappedRangeValueClamped(FVector2D(WindowPos.Y, WindowPos.Y + WindowSize.Y), FVector2D(0.0, 1.0), MouseLastPosition.Y);

				CursorVeloTarget = (MousePosition - MouseLastPosition) * DeltaTime;
				CursorVelocity = FMath::Vector2DInterpTo(CursorVelocity, CursorVeloTarget, DeltaTime, 0.1f);
				if (CNInputProcessor->bIsDragging && FocusedGraph)
				{
					CursorDragOffset += CursorVeloTarget;
					GrabAnimationValue = FMath::Clamp(GrabAnimationValue + DeltaTime * 3.5f, 0.0f, 1.0f);
				}
				else
				{
					GrabAnimationValue = FMath::Clamp(GrabAnimationValue - DeltaTime * 3.5f, 0.0f, 1.0f);
				}
				float ApplicationScale = FSlateApplication::Get().GetApplicationScale();
				ApplicationScale *= Window->GetDPIScaleFactor();
				FCNUniformBuffer::Set("ApplicationScale", ApplicationScale);
				FCNUniformBuffer::Set("GrabAnimation", Settings->AnimateBackground? GrabAnimationValue : 0);
				FCNUniformBuffer::Set("CursorX", MousePosition.X);
				FCNUniformBuffer::Set("CursorY", MousePosition.Y);
				FCNUniformBuffer::Set("WindowSizeX", WindowSize.X);
				FCNUniformBuffer::Set("WindowSizeY", WindowSize.Y);
				FCNUniformBuffer::Set("DragOffsetX", CursorDragOffset.X);
				FCNUniformBuffer::Set("DragOffsetY", CursorDragOffset.Y);
				FCNUniformBuffer::Set("VelocityX", CursorVelocity.X);
				FCNUniformBuffer::Set("VelocityY", CursorVelocity.Y);
				FCNUniformBuffer::Update();
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
	USlateBrushAsset* RegularBody = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_RegularBody.SB_RegularBody'"));
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 7
	//5.7 fixed the padding of node color spill. The old brush becomes legacy.
	USlateBrushAsset* RegularColor = LoadObject<USlateBrushAsset>(nullptr, TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_RegularColor.SB_RegularColor'"));
#else
	USlateBrushAsset* RegularColor = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_RegularColor_Legacy.SB_RegularColor_Legacy'"));
#endif
	USlateBrushAsset* RegularSelection = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_RegularSelection.SB_RegularSelection'"));
	if (RegularBody && RegularColor && RegularSelection)
	{
		Style->Set("Graph.Node.Body", new FSlateBrush(RegularBody->Brush));
		Style->Set("Graph.Node.TintedBody", new BOX_BRUSH("Blank", FMargin(0.25f)));
		Style->Set("Graph.Node.ColorSpill", new FSlateBrush(RegularColor->Brush));
		Style->Set("Graph.Node.TitleHighlight", new BOX_BRUSH("Blank", FMargin(0.25f)));
		Style->Set("Graph.Node.TitleGloss", new BOX_BRUSH("Blank", FMargin(0.25f)));
		Style->Set("Graph.Node.ShadowSelected", new FSlateBrush(RegularSelection->Brush));
		Style->Set("Graph.Node.Shadow", new BOX_BRUSH("RegularNode_shadow", FMargin(0.48f, 0.48f)));
		Style->Set("Graph.CollapsedNode.Body", new FSlateBrush(RegularBody->Brush));
		Style->Set("Graph.CollapsedNode.BodyColorSpill", new IMAGE_BRUSH("", FVector2D(1.0f), FLinearColor(0,0,0,0)));
	}
	//Var node styles
	USlateBrushAsset* VarBody = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_VarBody.SB_VarBody'"));
	USlateBrushAsset* VarColor = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_VarColor.SB_VarColor'"));
	USlateBrushAsset* VarSelection = LoadObject<USlateBrushAsset>(nullptr,TEXT("/Script/Engine.SlateBrushAsset'/CrystalNodes/SB_VarSelection.SB_VarSelection'"));
	if (VarBody && VarColor && VarSelection)
	{
		Style->Set("Graph.VarNode.Body", new FSlateBrush(VarBody->Brush));
		Style->Set("Graph.VarNode.ColorSpill", new FSlateBrush(VarColor->Brush));
		Style->Set("Graph.VarNode.Gloss", new BOX_BRUSH("Blank", FMargin(0.25)));
		Style->Set("Graph.VarNode.ShadowSelected", new FSlateBrush(VarSelection->Brush));
		Style->Set("Graph.VarNode.Shadow", new BOX_BRUSH("VarNode_shadow", FMargin(0.35f, 0.25f)));
	}
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

void FCrystalNodesModule::OnSlateFocusChange(const FFocusEvent& FocusEvent, const FWeakWidgetPath& WeakWidgetPath, const TSharedPtr<SWidget>& Widget, const FWidgetPath& WidgetPath, const TSharedPtr<SWidget>& Shared)
{
	auto Settings = GetDefault<UCrystalNodesSettings>();

	if (Shared && Shared->GetType() == FName("SGraphPanel"))
	{
		if (TSharedPtr<SGraphPanel> Graph = StaticCastSharedPtr<SGraphPanel>(Shared))
		{
			if (Graph->GetParentWidget()->GetParentWidget()->GetParentWidget()->GetParentWidget()->GetParentWidget()->GetType() == FName("SBorder"))
			{
				FocusedGraph = Graph.Get();
				GraphSelectionAnimValue = 0.0;
				if (!Settings->HideGraphText)
				{
					return;
				}
				if (TSharedPtr<STextBlock> GraphText = StaticCastSharedRef<STextBlock>(Graph->GetParentWidget()->GetAllChildren()->GetChildAt(3)))
				{
					FText Text = FText::AsCultureInvariant(TEXT(""));
					GraphText->SetText(Text);
				}
				return;
			}
		}
	}
	if (Widget)
	{
		if (Widget->GetType() == FName("SGraphPanel"))
		{
			UUnrealEditorSubsystem* EditorSys = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
			FCNUniformBuffer::Set("FocusAnimation", 0);
		}
	}
	FocusedGraph = nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCrystalNodesModule, CrystalNode)

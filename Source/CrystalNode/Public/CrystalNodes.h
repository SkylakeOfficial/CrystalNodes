// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/IInputProcessor.h"

struct FCrystalInputProcessor :public IInputProcessor
{
public:
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override {return false;}
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override {return false;}
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override {return false;}
	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent) override {return false;}
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
	virtual bool HandleMouseButtonDownEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonUpEvent( FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	bool bIsDragging = false;
};

//The uniform buffer of crystal nodes material parameters
struct FCNUniformBuffer
{
private:
	static constexpr int32 BufferSize = 8;
	static const TMap<FName, FUintPoint>& GetParamDefinition()
	{
		static const TMap<FName, FUintPoint> Definition = {
			{"CursorX",				FUintPoint(0,0b1000u)},
			{"CursorY",				FUintPoint(0,0b0100u)},
			{"WindowSizeX",			FUintPoint(0,0b0010u)},
			{"WindowSizeY",			FUintPoint(0,0b0001u)},
			
			{"GrabAnimation",		FUintPoint(1,0b1000u)},
			{"FocusAnimation",		FUintPoint(1,0b0100u)},
			{"ApplicationScale",	FUintPoint(1,0b0010u)},
			{"UserScale",			FUintPoint(1,0b0001u)},

			{"UserNodeOutline",		FUintPoint(2,0b1000u)},
			{"UserVarNodeOutline",	FUintPoint(2,0b0100u)},
			{"UserNodeExpand",		FUintPoint(2,0b0010u)},
			{"UserVarNodeExpand",	FUintPoint(2,0b0001u)},
			
			{"UserThemeColor",		FUintPoint(3,0b1110u)},
			
			{"UserLightColor",		FUintPoint(4,0b1110u)},
			
			{"UserEnablePhong",		FUintPoint(5,0b1000u)},
			{"UserEnableCursorDF",	FUintPoint(5,0b0100u)},

			{"UserBlueNoiseAmount",	FUintPoint(5,0b0001u)},
						
			{"DragOffsetX",			FUintPoint(6,0b1000u)},
			{"DragOffsetY",			FUintPoint(6,0b0100u)},
			{"VelocityX",			FUintPoint(6,0b0010u)},
			{"VelocityY",			FUintPoint(6,0b0001u)},
		};
		return Definition;
	}
	static TArray<FFloat16Color> ParamData;
	static bool bEverInitialized;
	static UTextureRenderTarget2D* RenderTarget;
	static void initialize();
public:
	static void Set(FName ParamName, const FFloat16& Param);
	static void Set(FName ParamName, const FFloat16Color& Param);
	static void Update();
};

class FCrystalNodesModule : public IModuleInterface
{
public:
	
	void OnSettingChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:
	void UpdateUserSettings();
	
	static void BeautifyEditor();
	
	FTickerDelegate TickDelegate;
	FTSTicker::FDelegateHandle TickDelegateHandle;
	FDelegateHandle SettingChangeDelegate;
	bool Tick(float DeltaTime);

	bool bEverBoundSlateDelegates = false;
	void OnSlateFocusChange(const FFocusEvent& FocusEvent, const FWeakWidgetPath& WeakWidgetPath, const TSharedPtr<SWidget>& Widget, const FWidgetPath& WidgetPath, const TSharedPtr<SWidget>& Shared);
	
	void BindSlateDelegates()
	{
		SlateHandle = FSlateApplication::Get().OnFocusChanging().AddRaw(this, &FCrystalNodesModule::OnSlateFocusChange);
		FSlateApplication::Get().RegisterInputPreProcessor(MakeShareable(CNInputProcessor));
		bEverBoundSlateDelegates = true;
		UpdateUserSettings();
	}
	FCrystalInputProcessor* CNInputProcessor = nullptr;
	FDelegateHandle SlateHandle;
	float GrabAnimationValue = 0;
	FVector2D CursorVelocity;
	FVector2D CursorVeloTarget;
	SWidget* FocusedGraph = nullptr;
	FVector2D CursorDragOffset;
	float GraphSelectionAnimValue = 0.0;
};

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

class FCrystalNodesModule : public IModuleInterface
{
public:
	
	void OnSettingChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:
	void SetupSettings();
	
	static void BeautifyEditor();
	
	FTickerDelegate TickDelegate;
	FTSTicker::FDelegateHandle TickDelegateHandle;
	FDelegateHandle SettingChangeDelegate;
	bool Tick(float DeltaTime);
	UMaterialParameterCollection* EditorMatParams = nullptr;

	bool bEverBoundSlateDelegates = false;
	void OnSlateFocusChange(const FFocusEvent& FocusEvent, const FWeakWidgetPath& WeakWidgetPath, const TSharedPtr<SWidget>& Widget, const FWidgetPath& WidgetPath, const TSharedPtr<SWidget>& Shared);
	
	void BindSlateDelegates()
	{
		SlateHandle = FSlateApplication::Get().OnFocusChanging().AddRaw(this, &FCrystalNodesModule::OnSlateFocusChange);
		FSlateApplication::Get().RegisterInputPreProcessor(MakeShareable(MyProcessor));
		bEverBoundSlateDelegates = true;
		SetupSettings();
	}
	FCrystalInputProcessor* MyProcessor = nullptr;
	FDelegateHandle SlateHandle;
	float IsDraggingVal = 0;
	FVector2D CursorVel;
	FVector2D CursorVelTarget;
	SWidget* FocusingGraph = nullptr;
	FVector2D CursorDragOffset;
	float GraphSelectionAnimVal = 0.0;
};



#pragma once

#include "CoreMinimal.h"
#include "CrystalNodesSettings.generated.h"

/**
 * 
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Crystal Nodes", Keywords = "Crystal Nodes"))
class CRYSTALNODES_API UCrystalNodesSettings : public UObject
{
	GENERATED_BODY()
#if WITH_EDITOR
public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSettingsChanged, UObject*, struct FPropertyChangedEvent&);
	FOnSettingsChanged& OnSettingChanged();
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	FOnSettingsChanged SettingsChangedDelegate;
	
	/**
	 * React to the mouse position
	 */
	UPROPERTY(config, EditAnywhere, Category= "Effects", DisplayName = "Cursor distance field")
	bool CursorDistanceField = true;
	/**
	 * Point light effect on cursor
	 */
	UPROPERTY(config, EditAnywhere, Category= "Effects", DisplayName = "Node point light")
	bool PhongLight = true;
	/**
	 * Outline width of nodes
	 */
	UPROPERTY(config, EditAnywhere, Category= "Effects", DisplayName = "Outline width of nodes")
	float NodeOutlineWidth = 0.5;
	/**
	 * Outline width of variable nodes
	 */
	UPROPERTY(config, EditAnywhere, Category= "Effects", DisplayName = "Outline width of variable nodes")
	float VarNodeOutlineWidth = 0.5;
	/**
	 * When mouse hovered, the width expansion of nodes
	 */
	UPROPERTY(config, EditAnywhere, Category= "Effects", DisplayName = "Node outline expansion amount")
	float NodeOutlineExpansion = 0.5;
	/**
	 * When mouse hovered, the width expansion of variable nodes
	 */
	UPROPERTY(config, EditAnywhere, Category= "Effects", DisplayName = "Variable outline expansion amount")
	float VarNodeOutlineExpansion = 0.5;
	
	
	/**
	 * Theme color of the nodes
	 */
	UPROPERTY(config, EditAnywhere, Category= "Color", DisplayName = "Theme color")
	FLinearColor ThemeColor = FColor::FromHex("719CFFFF");
	/**
	 * Color of the point light
	 */
	UPROPERTY(config, EditAnywhere, Category= "Color", DisplayName = "Light color")
	FLinearColor LightColor = FColor::FromHex("8399ACFF");
	/**
	 * Blue noise applied to nodes to enrich the visual quality
	 */
	UPROPERTY(config, EditAnywhere, Category= "Color", DisplayName = "Blue noise amount", meta=(UIMax = 1, UIMin = 0))
	float BlueNoiseAmount = 0.2;
	
	/**
	 * Let the graph background react to mouse clicks
	 */
	UPROPERTY(config, EditAnywhere, Category= "Misc", DisplayName = "Animate background")
	bool AnimateBackground = true;
	/**
	 * Custom scale of the background dot grid
	 */
	UPROPERTY(config, EditAnywhere, Category= "Misc", DisplayName = "Grid scale", meta=(UIMax = 2, UIMin = 0.5))
	float GridScale = 1.0;
	/**
	 * Hide the graph type text at the right bottom corner
	 */
	UPROPERTY(config, EditAnywhere, Category= "Misc", DisplayName = "Hide graph text")
	bool HideGraphText = true;

#endif

};

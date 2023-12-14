

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CrystalNodesSettings.generated.h"

/**
 * 
 */
UCLASS(Config = "CrystalNodes",DefaultConfig, meta = (DisplayName = "CrystalNodes"))
class CRYSTALNODES_API UCrystalNodesSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	/**
	 * Make nodes glow when close to the mouse cursor 
	 */
	UPROPERTY(config, EditAnywhere,DisplayName = "Enable cursor glow")
	bool EnableCursorGlow = true;
	/**
	 * Fancy edge light on the background of graph 
	 */
	UPROPERTY(config, EditAnywhere,DisplayName = "Enable graph edge light")
	bool EnableEdgeLight = true;
	/**
	 * Simulate a point light on the cursor, making nodes kind of skeuomorphic
	 */
	UPROPERTY(config, EditAnywhere,DisplayName = "Simulate light effect")
	bool EnableSimLight = true;

};

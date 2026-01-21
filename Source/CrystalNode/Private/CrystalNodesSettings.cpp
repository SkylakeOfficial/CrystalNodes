


#include "CrystalNodesSettings.h"

UCrystalNodesSettings::FOnSettingsChanged& UCrystalNodesSettings::OnSettingChanged()
{
	return SettingsChangedDelegate;
}

void UCrystalNodesSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	SettingsChangedDelegate.Broadcast(this, PropertyChangedEvent);
}

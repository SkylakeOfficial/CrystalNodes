#pragma once

#include "IDetailCustomization.h"

class FCrystalNodesSettingsCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
    
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	FReply OnSetupClicked();
	FReply OnResetClicked();
};
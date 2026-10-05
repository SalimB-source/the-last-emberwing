#include "Emberwing.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogEmberwing, Log, All);

class FEmberwingModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		UE_LOG(LogEmberwing, Display, TEXT("[Emberwing] module de gameplay charge (lumieres + rig procedural)"));
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FEmberwingModule, Emberwing, "Emberwing");

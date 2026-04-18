#include "Util/Logger/Logger.hpp"
#include "Hooks/Hooks.hpp"
#include "Version.hpp"

#include "CBPC/Config/Config.hpp"

namespace {
	void InitializeMessaging() {

		if (!SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* a_Message) {

			switch (a_Message->type) {

				// Called when all game data has been found.
				case SKSE::MessagingInterface::kInputLoaded:{
					//TODO: Function Calls To...
					//SetupReceptors();
					break;
				}

				// All ESM/ESL/ESP plugins have loaded, main menu is now active.
				case SKSE::MessagingInterface::kDataLoaded:{

					CBP::loadSystemConfig();
					CBP::loadMasterConfig();
					CBP::loadConfig();
					CBP::loadBounceInterpolationConfig();
					CBP::loadCollisionConfig();
					CBP::loadExtraCollisionConfig();
					CBP::loadCollisionInterpolationConfig();
					CBP::LoadPlayerCollisionEventConfig();

					break;
				}

				default:{}
			}
			})) {
			Util::Win32::ReportAndExit("Unable to register message listener.");
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface * a_SKSE) {

	SKSE::Init(a_SKSE);
	logger::Initialize();
	logger::SetLevel("trace");
	Hooks::Install();

	InitializeMessaging();

	//TODO:
	//Register Papyrus Interface
	//Register for TESEquipEvent



	logger::info("SKSEPluginLoad OK");

	return true;
}

SKSEPluginInfo(
	.Version = Plugin::ModVersion,
	.Name = Plugin::ModName,
	.Author = "BingusEx",
	.StructCompatibility = SKSE::StructCompatibility::Independent,
	.RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary
);
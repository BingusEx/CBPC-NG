#include "Console/ConsoleManager.hpp"


/*
 if (_strnicmp(buffer, "reload", MAX_PATH) == 0)
	{
		if (tuningModeCollision == 0)
		{
			Console_Print("Reload CBPC Master / Collision / Config files");

			consoleConfigReload.store(true);
		}
	}
	else if (_strnicmp(buffer, "sysreload", MAX_PATH) == 0)
	{
		Console_Print("Reload CBPC system file");
		loadSystemConfig();
		LOG_ERR("Loaded System config");
		return true;
	}
	else if (_strnicmp(buffer, "pause", MAX_PATH) == 0)
	{
		Console_Print("CBPC paused. Call \"cbpc start\" to resume.");
		modPaused.store(true);
		return true;
	}
	else if (_strnicmp(buffer, "start", MAX_PATH) == 0)
	{
		Console_Print("CBPC started. Call \"cbpc pause\" to pause.");
		modPaused.store(false);
		return true;
	}

 */


namespace CBP {

	void ConsoleManager::RegisterCommand(std::string_view a_cmdName, const std::function<void()>& a_callback, const std::string& a_desc) {
		std::string name(a_cmdName);
		RegisteredCommands.try_emplace(name, a_callback, a_desc);
		logger::info("Registered Console Command \"{} {}\"", Default_Preffix, name);
	}

	bool ConsoleManager::Process(const std::string& a_msg) {

		if (RegisteredCommands.empty()) return false;

		//Convert to invariant and trim
		std::stringstream Msg(Util::Text::Trim(Util::Text::ToLower(a_msg)));

		std::vector<std::string> Args{};
		std::string TmpArg;

		while (Msg >> TmpArg) {

			//If subcommands are ever needed just increase this value
			if (Args.size() == 2) {
				break;
			}

			Args.emplace_back(TmpArg);

			//no "gts" ? then its not our problem to deal with
			if (Args.at(0) != Default_Preffix) {
				return false;
			}
		}

		//if 1 arg show help
		if (Args.size() < 2) {
			CMD_Help();
			return true;
		}

		for (const auto& registered_command : RegisteredCommands) {
			if (registered_command.first == Args.at(1)) {
				if (registered_command.second.callback) {
					registered_command.second.callback();
					return true;
				}
				else {
					logger::warn("Command {} has no function assigned to it", registered_command.first);
					return false;
				}
			}
		}

		Util::Text::Print("Command not found type {} help for a list of commands.", Default_Preffix);
		return true;
	}


	void ConsoleManager::CMD_Help() {
		Util::Text::Print("--- List of available commands ---");

		for (const auto& key : RegisteredCommands) {
			Util::Text::Print("* {} {} - {} ", Default_Preffix, key.first, key.second.desc);
		}
	}


	void ConsoleManager::Init() {
		RegisterCommand("help", CMD_Help, "Show this list");
	}
}


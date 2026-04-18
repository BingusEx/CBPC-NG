#include "CBPC/Config/Config.hpp"

namespace CBP {

	bool compareConfigs(const SpecificNPCConfig& config1, const SpecificNPCConfig& config2) {
		return config1.ConditionPriority > config2.ConditionPriority
			|| (config1.ConditionPriority == config2.ConditionPriority && config1.conditions.AndItems.size() > config2.conditions.AndItems.size());
	}

	bool compareBounceConfigs(const SpecificNPCBounceConfig& config1, const SpecificNPCBounceConfig& config2) {
		return config1.ConditionPriority > config2.ConditionPriority
			|| (config1.ConditionPriority == config2.ConditionPriority && config1.conditions.AndItems.size() > config2.conditions.AndItems.size());
	}

	uint8_t  GetLoadedModIndex(const char* modName) {
		RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
		if (dataHandler != nullptr) {

			const RE::TESFile* modInfo = dataHandler->LookupModByName(modName);

			if (!modInfo->GetCombinedIndex()) return 0xFF;

			return modInfo->GetCombinedIndex();
		}

		return -1;
	}

	RE::TESForm* ParseFormFromSplitted(std::vector<std::string>& splittedByPlugin) {
		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) return nullptr;

		if (splittedByPlugin.size() > 1)
		{
			const std::uint32_t localFormID = getHex(splittedByPlugin[1].c_str());
			return dataHandler->LookupForm(localFormID, splittedByPlugin[0]);
		}
		else if (!splittedByPlugin.empty())
		{
			const std::uint32_t formID = getHex(splittedByPlugin[0].c_str());
			return formID ? RE::TESForm::LookupByID(formID) : nullptr;
		}

		return nullptr;
	}

	uint32_t GetFormIdFromString(std::string str) {
		std::ranges::transform(str, str.begin(), ::tolower);

		trim(str);

		std::vector<std::string> splittedByPlugin = split(str, '|');

		RE::TESForm* form = ParseFormFromSplitted(splittedByPlugin);
		if (form != nullptr) return form->formID;
		return 0;
	}

	void DecideConditionType(const std::string& line, uint32_t& id, std::string& str, ConditionType& type) {
		std::vector<std::string> splittedMain = splitMulti(line, "()[]{}");

		if (splittedMain.size() > 0) {
			if (splittedMain[0] == "IsRaceFormId") {
				type = ConditionType::IsRaceFormId;
				if (splittedMain.size() > 1) {
					id = GetFormIdFromString(splittedMain[1]);
				}
			}
			else if (splittedMain[0] == "IsRaceName") {
				type = ConditionType::IsRaceName;
				if (splittedMain.size() > 1){
					str = splittedMain[1];
				}
			}
			else if (splittedMain[0] == "ActorName") {
				type = ConditionType::ActorName;
				if (splittedMain.size() > 1){
					str = splittedMain[1];
				}
			}
			else if (splittedMain[0] == "ActorFormId")
			{
				type = ConditionType::ActorFormId;
				if (splittedMain.size() > 1)
				{
					id = GetFormIdFromString(splittedMain[1]);
				}
			}
			else if (splittedMain[0] == "IsInFaction")
			{
				type = ConditionType::IsInFaction;
				if (splittedMain.size() > 1)
				{
					id = GetFormIdFromString(splittedMain[1]);
				}
			}
			else if (splittedMain[0] == "IsPlayerTeammate")
			{
				type = ConditionType::IsPlayerTeammate;
			}
			else if (splittedMain[0] == "IsFemale")
			{
				type = ConditionType::IsFemale;
			}
			else if (splittedMain[0] == "IsMale")
			{
				type = ConditionType::IsMale;
			}
			else if (splittedMain[0] == "IsPlayer")
			{
				type = ConditionType::IsPlayer;
			}
			else if (splittedMain[0] == "HasKeywordId")
			{
				type = ConditionType::HasKeywordId;
				if (splittedMain.size() > 1)
				{
					id = GetFormIdFromString(splittedMain[1]);
				}
			}
			else if (splittedMain[0] == "HasKeywordName")
			{
				type = ConditionType::HasKeywordName;
				if (splittedMain.size() > 1)
				{
					str = splittedMain[1];
				}
			}
			else if (splittedMain[0] == "RaceHasKeywordId")
			{
				type = ConditionType::RaceHasKeywordId;
				if (splittedMain.size() > 1)
				{
					id = GetFormIdFromString(splittedMain[1]);
				}
			}
			else if (splittedMain[0] == "RaceHasKeywordName")
			{
				type = ConditionType::RaceHasKeywordName;
				if (splittedMain.size() > 1)
				{
					str = splittedMain[1];
				}
			}
			else if (splittedMain[0] == "IsActorBase")
			{
				type = ConditionType::IsActorBase;
				if (splittedMain.size() > 1)
				{
					id = GetFormIdFromString(splittedMain[1]);
				}
			}
			else if (splittedMain[0] == "IsUnique")
			{
				type = ConditionType::IsUnique;
			}
			else if (splittedMain[0] == "IsVoiceType")
			{
				type = ConditionType::IsVoiceType;
				if (splittedMain.size() > 1)
				{
					id = GetFormIdFromString(splittedMain[1]);
				}
			}
			else if (splittedMain[0] == "IsCombatStyle")
			{
				type = ConditionType::IsCombatStyle;
				if (splittedMain.size() > 1)
				{
					id = GetFormIdFromString(splittedMain[1]);
				}
			}
			else if (splittedMain[0] == "IsClass")
			{
				type = ConditionType::IsClass;
				if (splittedMain.size() > 1)
				{
					id = GetFormIdFromString(splittedMain[1]);
				}
			}
		}
	}

	Conditions ParseConditions(std::string& str) {
		Conditions newConditions;

		std::vector<std::string> splittedANDs = split(str, "AND");

		for (auto& strAnd : splittedANDs)
		{
			if (Contains(strAnd, " OR "))
			{
				std::vector<std::string> splittedORs = split(strAnd, "OR");

				ConditionItem cItem;

				for (auto& strOr : splittedORs)
				{
					ConditionItem oItem;
					if (stringStartsWith(strOr, "not "))
					{
						oItem.Not = true;

						strOr.erase(0, 3);

						trim(strOr);
					}

					oItem.single = true;
					DecideConditionType(strOr, oItem.id, oItem.str, oItem.type);

					cItem.OrItems.emplace_back(oItem);
				}
				cItem.single = false;

				newConditions.AndItems.emplace_back(cItem);
			}
			else
			{
				ConditionItem cItem;
				if (stringStartsWith(strAnd, "not "))
				{
					cItem.Not = true;

					strAnd.erase(0, 3);

					trim(strAnd);
				}

				cItem.single = true;
				DecideConditionType(strAnd, cItem.id, cItem.str, cItem.type);

				newConditions.AndItems.emplace_back(cItem);
			}
		}

		return newConditions;
	}

	void loadConfig() {
		//logger.info("loadConfig\n");

		//Console_Print("Reading CBP Config");
		std::string	runtimeDirectory = std::filesystem::current_path().string();
		if (!runtimeDirectory.empty())
		{
			std::string configPath = runtimeDirectory + "\\Data\\SKSE\\Plugins\\";

			Globals::config.clear();
			Globals::config0weight.clear();
			Globals::specificNPCBounceConfigList.clear();

			//Set default values
			for (auto& it : Globals::configMap) {
				//100 weight
				Globals::config[it.second]["stiffness"] = 0.0f;
				Globals::config[it.second]["stiffnessX"] = 0.0f;
				Globals::config[it.second]["stiffnessY"] = 0.0f;
				Globals::config[it.second]["stiffnessZ"] = 0.0f;
				Globals::config[it.second]["stiffnessXRot"] = 0.0f;
				Globals::config[it.second]["stiffnessYRot"] = 0.0f;
				Globals::config[it.second]["stiffnessZRot"] = 0.0f;
				Globals::config[it.second]["stiffness2"] = 0.0f;
				Globals::config[it.second]["stiffness2X"] = 0.0f;
				Globals::config[it.second]["stiffness2Y"] = 0.0f;
				Globals::config[it.second]["stiffness2Z"] = 0.0f;
				Globals::config[it.second]["stiffness2XRot"] = 0.0f;
				Globals::config[it.second]["stiffness2YRot"] = 0.0f;
				Globals::config[it.second]["stiffness2ZRot"] = 0.0f;
				Globals::config[it.second]["damping"] = 0.0f;
				Globals::config[it.second]["dampingX"] = 0.0f;
				Globals::config[it.second]["dampingY"] = 0.0f;
				Globals::config[it.second]["dampingZ"] = 0.0f;
				Globals::config[it.second]["dampingXRot"] = 0.0f;
				Globals::config[it.second]["dampingYRot"] = 0.0f;
				Globals::config[it.second]["dampingZRot"] = 0.0f;
				Globals::config[it.second]["maxoffset"] = 0.0f;
				Globals::config[it.second]["Xmaxoffset"] = 5.0f;
				Globals::config[it.second]["Xminoffset"] = -5.0f;
				Globals::config[it.second]["Ymaxoffset"] = 5.0f;
				Globals::config[it.second]["Yminoffset"] = -5.0f;
				Globals::config[it.second]["Zmaxoffset"] = 5.0f;
				Globals::config[it.second]["Zminoffset"] = -5.0f;
				Globals::config[it.second]["XmaxoffsetRot"] = 0.0f;
				Globals::config[it.second]["XminoffsetRot"] = 0.0f;
				Globals::config[it.second]["YmaxoffsetRot"] = 0.0f;
				Globals::config[it.second]["YminoffsetRot"] = 0.0f;
				Globals::config[it.second]["ZmaxoffsetRot"] = 0.0f;
				Globals::config[it.second]["ZminoffsetRot"] = 0.0f;
				Globals::config[it.second]["Xdefaultoffset"] = 0.0f;
				Globals::config[it.second]["Ydefaultoffset"] = 0.0f;
				Globals::config[it.second]["Zdefaultoffset"] = 0.0f;
				Globals::config[it.second]["cogOffset"] = 0.0f;
				Globals::config[it.second]["gravityBias"] = 0.0f;
				Globals::config[it.second]["gravityCorrection"] = 0.0f;
				Globals::config[it.second]["timetick"] = 4.0f;
				Globals::config[it.second]["timetickRot"] = 0.0f;
				Globals::config[it.second]["linearX"] = 0.0f;
				Globals::config[it.second]["linearY"] = 0.0f;
				Globals::config[it.second]["linearZ"] = 0.0f;
				Globals::config[it.second]["rotationalX"] = 0.0f;
				Globals::config[it.second]["rotationalY"] = 0.0f;
				Globals::config[it.second]["rotationalZ"] = 0.0f;
				Globals::config[it.second]["linearXrotationX"] = 0.0f;
				Globals::config[it.second]["linearXrotationY"] = 1.0f;
				Globals::config[it.second]["linearXrotationZ"] = 0.0f;
				Globals::config[it.second]["linearYrotationX"] = 0.0f;
				Globals::config[it.second]["linearYrotationY"] = 0.0f;
				Globals::config[it.second]["linearYrotationZ"] = 1.0f;
				Globals::config[it.second]["linearZrotationX"] = 1.0f;
				Globals::config[it.second]["linearZrotationY"] = 0.0f;
				Globals::config[it.second]["linearZrotationZ"] = 0.0f;
				Globals::config[it.second]["timeStep"] = 1.0f;
				Globals::config[it.second]["timeStepRot"] = 0.0f;
				Globals::config[it.second]["linearXspreadforceY"] = 0.0f;
				Globals::config[it.second]["linearXspreadforceZ"] = 0.0f;
				Globals::config[it.second]["linearYspreadforceX"] = 0.0f;
				Globals::config[it.second]["linearYspreadforceZ"] = 0.0f;
				Globals::config[it.second]["linearZspreadforceX"] = 0.0f;
				Globals::config[it.second]["linearZspreadforceY"] = 0.0f;
				Globals::config[it.second]["rotationXspreadforceY"] = 0.0f;
				Globals::config[it.second]["rotationXspreadforceZ"] = 0.0f;
				Globals::config[it.second]["rotationYspreadforceX"] = 0.0f;
				Globals::config[it.second]["rotationYspreadforceZ"] = 0.0f;
				Globals::config[it.second]["rotationZspreadforceX"] = 0.0f;
				Globals::config[it.second]["rotationZspreadforceY"] = 0.0f;
				Globals::config[it.second]["forceMultipler"] = 1.0f;
				Globals::config[it.second]["gravityInvertedCorrection"] = 0.0f;
				Globals::config[it.second]["gravityInvertedCorrectionStart"] = 0.0f;
				Globals::config[it.second]["gravityInvertedCorrectionEnd"] = 0.0f;
				Globals::config[it.second]["breastClothedPushup"] = 0.0f;
				Globals::config[it.second]["breastLightArmoredPushup"] = 0.0f;
				Globals::config[it.second]["breastHeavyArmoredPushup"] = 0.0f;
				Globals::config[it.second]["breastClothedAmplitude"] = 1.0f;
				Globals::config[it.second]["breastLightArmoredAmplitude"] = 1.0f;
				Globals::config[it.second]["breastHeavyArmoredAmplitude"] = 1.0f;
				Globals::config[it.second]["collisionFriction"] = 0.8f;
				Globals::config[it.second]["collisionPenetration"] = 0.0f;
				Globals::config[it.second]["collisionMultipler"] = 1.0f;
				Globals::config[it.second]["collisionMultiplerRot"] = 1.0f;
				Globals::config[it.second]["collisionElastic"] = 1.0f;
				Globals::config[it.second]["collisionElasticConstraints"] = 1.0f;
				Globals::config[it.second]["collisionXmaxoffset"] = 100.0f;
				Globals::config[it.second]["collisionXminoffset"] = -100.0f;
				Globals::config[it.second]["collisionYmaxoffset"] = 100.0f;
				Globals::config[it.second]["collisionYminoffset"] = -100.0f;
				Globals::config[it.second]["collisionZmaxoffset"] = 100.0f;
				Globals::config[it.second]["collisionZminoffset"] = -100.0f;
				Globals::config[it.second]["amplitude"] = 1.0f;

				//0 weight
				Globals::config0weight[it.second]["stiffness"] = 0.0f;
				Globals::config0weight[it.second]["stiffnessX"] = 0.0f;
				Globals::config0weight[it.second]["stiffnessY"] = 0.0f;
				Globals::config0weight[it.second]["stiffnessZ"] = 0.0f;
				Globals::config0weight[it.second]["stiffnessXRot"] = 0.0f;
				Globals::config0weight[it.second]["stiffnessYRot"] = 0.0f;
				Globals::config0weight[it.second]["stiffnessZRot"] = 0.0f;
				Globals::config0weight[it.second]["stiffness2"] = 0.0f;
				Globals::config0weight[it.second]["stiffness2X"] = 0.0f;
				Globals::config0weight[it.second]["stiffness2Y"] = 0.0f;
				Globals::config0weight[it.second]["stiffness2Z"] = 0.0f;
				Globals::config0weight[it.second]["dampingXRot"] = 0.0f;
				Globals::config0weight[it.second]["dampingYRot"] = 0.0f;
				Globals::config0weight[it.second]["dampingZRot"] = 0.0f;
				Globals::config0weight[it.second]["damping"] = 0.0f;
				Globals::config0weight[it.second]["dampingX"] = 0.0f;
				Globals::config0weight[it.second]["dampingY"] = 0.0f;
				Globals::config0weight[it.second]["dampingZ"] = 0.0f;
				Globals::config0weight[it.second]["dampingXRot"] = 0.0f;
				Globals::config0weight[it.second]["dampingYRot"] = 0.0f;
				Globals::config0weight[it.second]["dampingZRot"] = 0.0f;
				Globals::config0weight[it.second]["maxoffset"] = 0.0f;
				Globals::config0weight[it.second]["Xmaxoffset"] = 5.0f;
				Globals::config0weight[it.second]["Xminoffset"] = -5.0f;
				Globals::config0weight[it.second]["Ymaxoffset"] = 5.0f;
				Globals::config0weight[it.second]["Yminoffset"] = -5.0f;
				Globals::config0weight[it.second]["Zmaxoffset"] = 5.0f;
				Globals::config0weight[it.second]["Zminoffset"] = -5.0f;
				Globals::config0weight[it.second]["XmaxoffsetRot"] = 0.0f;
				Globals::config0weight[it.second]["XminoffsetRot"] = 0.0f;
				Globals::config0weight[it.second]["YmaxoffsetRot"] = 0.0f;
				Globals::config0weight[it.second]["YminoffsetRot"] = 0.0f;
				Globals::config0weight[it.second]["ZmaxoffsetRot"] = 0.0f;
				Globals::config0weight[it.second]["ZminoffsetRot"] = 0.0f;
				Globals::config0weight[it.second]["Xdefaultoffset"] = 0.0f;
				Globals::config0weight[it.second]["Ydefaultoffset"] = 0.0f;
				Globals::config0weight[it.second]["Zdefaultoffset"] = 0.0f;
				Globals::config0weight[it.second]["cogOffset"] = 0.0f;
				Globals::config0weight[it.second]["gravityBias"] = 0.0f;
				Globals::config0weight[it.second]["gravityCorrection"] = 0.0f;
				Globals::config0weight[it.second]["timetick"] = 4.0f;
				Globals::config0weight[it.second]["timetickRot"] = 0.0f;
				Globals::config0weight[it.second]["linearX"] = 0.0f;
				Globals::config0weight[it.second]["linearY"] = 0.0f;
				Globals::config0weight[it.second]["linearZ"] = 0.0f;
				Globals::config0weight[it.second]["rotationalX"] = 0.0f;
				Globals::config0weight[it.second]["rotationalY"] = 0.0f;
				Globals::config0weight[it.second]["rotationalZ"] = 0.0f;
				Globals::config0weight[it.second]["linearXrotationX"] = 0.0f;
				Globals::config0weight[it.second]["linearXrotationY"] = 1.0f;
				Globals::config0weight[it.second]["linearXrotationZ"] = 0.0f;
				Globals::config0weight[it.second]["linearYrotationX"] = 0.0f;
				Globals::config0weight[it.second]["linearYrotationY"] = 0.0f;
				Globals::config0weight[it.second]["linearYrotationZ"] = 1.0f;
				Globals::config0weight[it.second]["linearZrotationX"] = 1.0f;
				Globals::config0weight[it.second]["linearZrotationY"] = 0.0f;
				Globals::config0weight[it.second]["linearZrotationZ"] = 0.0f;
				Globals::config0weight[it.second]["timeStep"] = 1.0f;
				Globals::config0weight[it.second]["timeStepRot"] = 0.0f;
				Globals::config0weight[it.second]["linearXspreadforceY"] = 0.0f;
				Globals::config0weight[it.second]["linearXspreadforceZ"] = 0.0f;
				Globals::config0weight[it.second]["linearYspreadforceX"] = 0.0f;
				Globals::config0weight[it.second]["linearYspreadforceZ"] = 0.0f;
				Globals::config0weight[it.second]["linearZspreadforceX"] = 0.0f;
				Globals::config0weight[it.second]["linearZspreadforceY"] = 0.0f;
				Globals::config0weight[it.second]["rotationXspreadforceY"] = 0.0f;
				Globals::config0weight[it.second]["rotationXspreadforceZ"] = 0.0f;
				Globals::config0weight[it.second]["rotationYspreadforceX"] = 0.0f;
				Globals::config0weight[it.second]["rotationYspreadforceZ"] = 0.0f;
				Globals::config0weight[it.second]["rotationZspreadforceX"] = 0.0f;
				Globals::config0weight[it.second]["rotationZspreadforceY"] = 0.0f;
				Globals::config0weight[it.second]["forceMultipler"] = 1.0f;
				Globals::config0weight[it.second]["gravityInvertedCorrection"] = 0.0f;
				Globals::config0weight[it.second]["gravityInvertedCorrectionStart"] = 0.0f;
				Globals::config0weight[it.second]["gravityInvertedCorrectionEnd"] = 0.0f;
				Globals::config0weight[it.second]["breastClothedPushup"] = 0.0f;
				Globals::config0weight[it.second]["breastLightArmoredPushup"] = 0.0f;
				Globals::config0weight[it.second]["breastHeavyArmoredPushup"] = 0.0f;
				Globals::config0weight[it.second]["breastClothedAmplitude"] = 1.0f;
				Globals::config0weight[it.second]["breastLightArmoredAmplitude"] = 1.0f;
				Globals::config0weight[it.second]["breastHeavyArmoredAmplitude"] = 1.0f;
				Globals::config0weight[it.second]["collisionFriction"] = 0.8f;
				Globals::config0weight[it.second]["collisionPenetration"] = 0.0f;
				Globals::config0weight[it.second]["collisionMultipler"] = 1.0f;
				Globals::config0weight[it.second]["collisionMultiplerRot"] = 1.0f;
				Globals::config0weight[it.second]["collisionElastic"] = 1.0f;
				Globals::config0weight[it.second]["collisionElasticConstraints"] = 1.0f;
				Globals::config0weight[it.second]["collisionXmaxoffset"] = 100.0f;
				Globals::config0weight[it.second]["collisionXminoffset"] = -100.0f;
				Globals::config0weight[it.second]["collisionYmaxoffset"] = 100.0f;
				Globals::config0weight[it.second]["collisionYminoffset"] = -100.0f;
				Globals::config0weight[it.second]["collisionZmaxoffset"] = 100.0f;
				Globals::config0weight[it.second]["collisionZminoffset"] = -100.0f;
				Globals::config0weight[it.second]["amplitude"] = 1.0f;
			}

			auto configList = get_all_files_names_within_folder(configPath.c_str());
			bool configOpened = false;
			for (std::size_t i = 0; i < configList.size(); i++)
			{
				std::string filename = configList.at(i);

				if (filename == "." || filename == "..")
					continue;

				if (stringStartsWith(filename, "cbpconfig") && (stringEndsWith(filename, ".txt") || stringEndsWith(filename, ".ini")))
				{
					std::string msg = "File found: " + filename;
					LOG("{}",msg.c_str());

					std::string filepath = configPath;
					filepath.append(filename);
					std::ifstream file(filepath);

					if (!file.is_open())
					{
						std::ranges::transform(filepath, filepath.begin(), ::tolower);
						file.open(filepath);
					}

					configOpened = true;

					SpecificNPCBounceConfig newNPCBounceConfig;

					bool withConditions = false;

					std::string conditions;
					if (file.is_open())
					{
						std::string line;
						std::string currentSetting;
						while (std::getline(file, line))
						{
							//trim(line);
							skipComments(line);
							trim(line);
							if (line.length() > 0)
							{
								if (Contains(line, "="))
								{
									std::string variableName;
									std::string variableValue = GetConfigSettingsStringValue(line, variableName);
									if (variableName == "Conditions")
									{
										withConditions = true;
										conditions = variableValue;
										LOG("Conditioned bounce config: {}", conditions.c_str());

										if (withConditions)
										{
											//Set default values
											for (auto& it : Globals::configMap)
											{
												//100 weight
												newNPCBounceConfig.config[it.second]["stiffness"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffnessX"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffnessY"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffnessZ"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffnessXRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffnessYRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffnessZRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffness2"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffness2X"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffness2Y"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffness2Z"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffness2XRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffness2YRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["stiffness2ZRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["damping"] = 0.0f;
												newNPCBounceConfig.config[it.second]["dampingX"] = 0.0f;
												newNPCBounceConfig.config[it.second]["dampingY"] = 0.0f;
												newNPCBounceConfig.config[it.second]["dampingZ"] = 0.0f;
												newNPCBounceConfig.config[it.second]["dampingXRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["dampingYRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["dampingZRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["maxoffset"] = 0.0f;
												newNPCBounceConfig.config[it.second]["Xmaxoffset"] = 5.0f;
												newNPCBounceConfig.config[it.second]["Xminoffset"] = -5.0f;
												newNPCBounceConfig.config[it.second]["Ymaxoffset"] = 5.0f;
												newNPCBounceConfig.config[it.second]["Yminoffset"] = -5.0f;
												newNPCBounceConfig.config[it.second]["Zmaxoffset"] = 5.0f;
												newNPCBounceConfig.config[it.second]["Zminoffset"] = -5.0f;
												newNPCBounceConfig.config[it.second]["XmaxoffsetRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["XminoffsetRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["YmaxoffsetRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["YminoffsetRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["ZmaxoffsetRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["ZminoffsetRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["Xdefaultoffset"] = 0.0f;
												newNPCBounceConfig.config[it.second]["Ydefaultoffset"] = 0.0f;
												newNPCBounceConfig.config[it.second]["Zdefaultoffset"] = 0.0f;
												newNPCBounceConfig.config[it.second]["cogOffset"] = 0.0f;
												newNPCBounceConfig.config[it.second]["gravityBias"] = 0.0f;
												newNPCBounceConfig.config[it.second]["gravityCorrection"] = 0.0f;
												newNPCBounceConfig.config[it.second]["timetick"] = 4.0f;
												newNPCBounceConfig.config[it.second]["timetickRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearX"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearY"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearZ"] = 0.0f;
												newNPCBounceConfig.config[it.second]["rotationalX"] = 0.0f;
												newNPCBounceConfig.config[it.second]["rotationalY"] = 0.0f;
												newNPCBounceConfig.config[it.second]["rotationalZ"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearXrotationX"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearXrotationY"] = 1.0f;
												newNPCBounceConfig.config[it.second]["linearXrotationZ"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearYrotationX"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearYrotationY"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearYrotationZ"] = 1.0f;
												newNPCBounceConfig.config[it.second]["linearZrotationX"] = 1.0f;
												newNPCBounceConfig.config[it.second]["linearZrotationY"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearZrotationZ"] = 0.0f;
												newNPCBounceConfig.config[it.second]["timeStep"] = 1.0f;
												newNPCBounceConfig.config[it.second]["timeStepRot"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearXspreadforceY"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearXspreadforceZ"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearYspreadforceX"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearYspreadforceZ"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearZspreadforceX"] = 0.0f;
												newNPCBounceConfig.config[it.second]["linearZspreadforceY"] = 0.0f;
												newNPCBounceConfig.config[it.second]["rotationXspreadforceY"] = 0.0f;
												newNPCBounceConfig.config[it.second]["rotationXspreadforceZ"] = 0.0f;
												newNPCBounceConfig.config[it.second]["rotationYspreadforceX"] = 0.0f;
												newNPCBounceConfig.config[it.second]["rotationYspreadforceZ"] = 0.0f;
												newNPCBounceConfig.config[it.second]["rotationZspreadforceX"] = 0.0f;
												newNPCBounceConfig.config[it.second]["rotationZspreadforceY"] = 0.0f;
												newNPCBounceConfig.config[it.second]["forceMultipler"] = 1.0f;
												newNPCBounceConfig.config[it.second]["gravityInvertedCorrection"] = 0.0f;
												newNPCBounceConfig.config[it.second]["gravityInvertedCorrectionStart"] = 0.0f;
												newNPCBounceConfig.config[it.second]["gravityInvertedCorrectionEnd"] = 0.0f;
												newNPCBounceConfig.config[it.second]["breastClothedPushup"] = 0.0f;
												newNPCBounceConfig.config[it.second]["breastLightArmoredPushup"] = 0.0f;
												newNPCBounceConfig.config[it.second]["breastHeavyArmoredPushup"] = 0.0f;
												newNPCBounceConfig.config[it.second]["breastClothedAmplitude"] = 1.0f;
												newNPCBounceConfig.config[it.second]["breastLightArmoredAmplitude"] = 1.0f;
												newNPCBounceConfig.config[it.second]["breastHeavyArmoredAmplitude"] = 1.0f;
												newNPCBounceConfig.config[it.second]["collisionFriction"] = 0.8f;
												newNPCBounceConfig.config[it.second]["collisionPenetration"] = 0.0f;
												newNPCBounceConfig.config[it.second]["collisionMultipler"] = 1.0f;
												newNPCBounceConfig.config[it.second]["collisionMultiplerRot"] = 1.0f;
												newNPCBounceConfig.config[it.second]["collisionElastic"] = 1.0f;
												newNPCBounceConfig.config[it.second]["collisionElasticConstraints"] = 1.0f;
												newNPCBounceConfig.config[it.second]["collisionXmaxoffset"] = 100.0f;
												newNPCBounceConfig.config[it.second]["collisionXminoffset"] = -100.0f;
												newNPCBounceConfig.config[it.second]["collisionYmaxoffset"] = 100.0f;
												newNPCBounceConfig.config[it.second]["collisionYminoffset"] = -100.0f;
												newNPCBounceConfig.config[it.second]["collisionZmaxoffset"] = 100.0f;
												newNPCBounceConfig.config[it.second]["collisionZminoffset"] = -100.0f;
												newNPCBounceConfig.config[it.second]["amplitude"] = 1.0f;

												//0 weight
												newNPCBounceConfig.config0weight[it.second]["stiffness"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffnessX"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffnessY"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffnessZ"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffnessXRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffnessYRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffnessZRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffness2"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffness2X"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffness2Y"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffness2Z"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffness2XRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffness2YRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["stiffness2ZRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["damping"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["dampingX"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["dampingY"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["dampingZ"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["dampingXRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["dampingYRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["dampingZRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["maxoffset"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["Xmaxoffset"] = 5.0f;
												newNPCBounceConfig.config0weight[it.second]["Xminoffset"] = -5.0f;
												newNPCBounceConfig.config0weight[it.second]["Ymaxoffset"] = 5.0f;
												newNPCBounceConfig.config0weight[it.second]["Yminoffset"] = -5.0f;
												newNPCBounceConfig.config0weight[it.second]["Zmaxoffset"] = 5.0f;
												newNPCBounceConfig.config0weight[it.second]["Zminoffset"] = -5.0f;
												newNPCBounceConfig.config0weight[it.second]["XmaxoffsetRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["XminoffsetRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["YmaxoffsetRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["YminoffsetRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["ZmaxoffsetRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["ZminoffsetRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["Xdefaultoffset"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["Ydefaultoffset"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["Zdefaultoffset"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["cogOffset"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["gravityBias"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["gravityCorrection"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["timetick"] = 4.0f;
												newNPCBounceConfig.config0weight[it.second]["timetickRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearX"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearY"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearZ"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["rotationalX"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["rotationalY"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["rotationalZ"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearXrotationX"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearXrotationY"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["linearXrotationZ"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearYrotationX"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearYrotationY"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearYrotationZ"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["linearZrotationX"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["linearZrotationY"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearZrotationZ"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["timeStep"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["timeStepRot"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearXspreadforceY"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearXspreadforceZ"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearYspreadforceX"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearYspreadforceZ"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearZspreadforceX"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["linearZspreadforceY"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["rotationXspreadforceY"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["rotationXspreadforceZ"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["rotationYspreadforceX"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["rotationYspreadforceZ"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["rotationZspreadforceX"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["rotationZspreadforceY"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["forceMultipler"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["gravityInvertedCorrection"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["gravityInvertedCorrectionStart"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["gravityInvertedCorrectionEnd"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["breastClothedPushup"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["breastLightArmoredPushup"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["breastHeavyArmoredPushup"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["breastClothedAmplitude"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["breastLightArmoredAmplitude"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["breastHeavyArmoredAmplitude"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["collisionFriction"] = 0.8f;
												newNPCBounceConfig.config0weight[it.second]["collisionPenetration"] = 0.0f;
												newNPCBounceConfig.config0weight[it.second]["collisionMultipler"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["collisionMultiplerRot"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["collisionElastic"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["collisionElasticConstraints"] = 1.0f;
												newNPCBounceConfig.config0weight[it.second]["collisionXmaxoffset"] = 100.0f;
												newNPCBounceConfig.config0weight[it.second]["collisionXminoffset"] = -100.0f;
												newNPCBounceConfig.config0weight[it.second]["collisionYmaxoffset"] = 100.0f;
												newNPCBounceConfig.config0weight[it.second]["collisionYminoffset"] = -100.0f;
												newNPCBounceConfig.config0weight[it.second]["collisionZmaxoffset"] = 100.0f;
												newNPCBounceConfig.config0weight[it.second]["collisionZminoffset"] = -100.0f;
												newNPCBounceConfig.config0weight[it.second]["amplitude"] = 1.0f;
											}
										}
									}
									else if (variableName == "Priority")
									{
										newNPCBounceConfig.ConditionPriority = GetConfigSettingsValue(line, variableName);
									}
								}
								else
								{
									std::vector<std::string> splittedMain = split(line, '.');

									if (splittedMain.size() > 1)
									{
										std::string tok0 = splittedMain[0];

										if (i != std::string::npos)
											line.erase(0, tok0.length() + 1);

										std::vector<std::string> splitted = splitNonEmpty(line, ' ');

										std::string tok1 = "";
										std::string tok2 = "";
										std::string tok3 = "";
										if (splitted.size() > 0)
										{
											tok1 = splitted[0];

											if (splitted.size() > 1)
											{
												tok2 = splitted[1];

												if (splitted.size() > 2)
												{
													tok3 = splitted[2];
												}
											}
										}

										if (withConditions)
										{
											if (tok0.size() > 0 && tok1.size() > 0 && tok2.size() > 0) {
												const float calcValue = atof(tok2.c_str());
												newNPCBounceConfig.config[tok0][tok1] = calcValue;
												newNPCBounceConfig.config0weight[tok0][tok1] = calcValue;
												LOG("Conditioned config[{}][{}] = {}", tok0.c_str(), tok1.c_str(), tok2.c_str());
											}
											if (tok0.size() > 0 && tok1.size() > 0 && tok3.size() > 0) {
												newNPCBounceConfig.config0weight[tok0][tok1] = atof(tok3.c_str());
												LOG("Conditioned 0 weight config[{}][{}] = {}", tok0.c_str(), tok1.c_str(), tok3.c_str());
											}
										}
										else
										{
											if (tok0.size() > 0 && tok1.size() > 0 && tok2.size() > 0) {
												const float calcValue = atof(tok2.c_str());
												Globals::config[tok0][tok1] = calcValue;
												Globals::config0weight[tok0][tok1] = calcValue;
												LOG("config[{}][{}] = {}", tok0.c_str(), tok1.c_str(), tok2.c_str());
											}
											if (tok0.size() > 0 && tok1.size() > 0 && tok3.size() > 0) {
												Globals::config0weight[tok0][tok1] = atof(tok3.c_str());
												LOG("0 weight config[{}][{}] = {}", tok0.c_str(), tok1.c_str(), tok3.c_str());
											}
										}
									}
								}
							}
						}
					}

					if (withConditions)
					{
						newNPCBounceConfig.conditions = ParseConditions(conditions);
						Globals::specificNPCBounceConfigList.emplace_back(newNPCBounceConfig);
					}
				}
			}

			if (Globals::specificNPCBounceConfigList.size() > 1)
			{
				std::ranges::sort(Globals::specificNPCBounceConfigList, compareBounceConfigs);
			}

			if (!configOpened)
				Globals::configReloadCount = 0;

			Globals::configReloadCount = Globals::config["Tuning"]["rate"];
		}
	}

	void loadBounceInterpolationConfig()
	{
		std::string	runtimeDirectory = std::filesystem::current_path().string();
		if (!runtimeDirectory.empty())
		{
			std::string configPath = runtimeDirectory + "\\Data\\SKSE\\Plugins\\";

			Globals::actorBounceInterpolationConfigMap_lock.lock();
			Globals::bounceInterpolationConfigMap.clear();

			auto configList = get_all_files_names_within_folder(configPath.c_str());
			bool configOpened = false;
			for (std::size_t i = 0; i < configList.size(); i++)
			{
				std::string filename = configList.at(i);

				if (filename == "." || filename == "..")
					continue;

				if ((stringStartsWith(filename, "cbpcbounceinterpolationconfig") || stringStartsWith(filename, "cbpcbounceınterpolationconfig")) && filename != "cbpcbounceinterpolationconfig.txt" && (stringEndsWith(filename, ".txt") || stringEndsWith(filename, ".ini")))
				{
					std::string msg = "File found: " + filename;
					LOG("{}", msg.c_str());

					std::string filepath = configPath;
					filepath.append(filename);
					std::ifstream file(filepath);

					if (!file.is_open())
					{
						std::ranges::transform(filepath, filepath.begin(), ::tolower);
						file.open(filepath);
					}

					configOpened = true;

					BounceInterpolationConfig newNPCBounceConfig;

					std::string uniqueName;

					if (file.is_open())
					{
						std::string line;
						std::string currentSetting;
						while (std::getline(file, line))
						{
							//trim(line);
							skipComments(line);
							trim(line);
							if (line.length() > 0)
							{
								if (Contains(line, "="))
								{
									std::string variableName;
									std::string variableValue = GetConfigSettingsStringValue(line, variableName);
									if (variableName == "UniqueName")
									{
										std::ranges::transform(variableValue, variableValue.begin(), ::tolower);
										uniqueName = variableValue;
										LOG_ERR("Bounce interpolation config: {}", uniqueName.c_str());
									}
								}
								else
								{
									std::vector<std::string> splittedMain = split(line, '.');

									if (splittedMain.size() > 1)
									{
										std::string tok0 = splittedMain[0];

										if (i != std::string::npos)
											line.erase(0, tok0.length() + 1);

										std::vector<std::string> splitted = splitNonEmpty(line, ' ');

										std::string tok1 = "";
										std::string tok2 = "";
										std::string tok3 = "";
										if (splitted.size() > 0)
										{
											tok1 = splitted[0];

											if (splitted.size() > 1)
											{
												tok2 = splitted[1];

												if (splitted.size() > 2)
												{
													tok3 = splitted[2];
												}
											}
										}

										if (tok0.size() > 0 && tok1.size() > 0 && tok2.size() > 0) {
											const float calcValue = atof(tok2.c_str());
											newNPCBounceConfig.config[tok0][tok1] = calcValue;
											newNPCBounceConfig.config0weight[tok0][tok1] = calcValue;
											LOG_ERR("Interpolation config[{}][{}] = {}", tok0.c_str(), tok1.c_str(), tok2.c_str());
										}
										if (tok0.size() > 0 && tok1.size() > 0 && tok3.size() > 0) {
											newNPCBounceConfig.config0weight[tok0][tok1] = atof(tok3.c_str());
											LOG_ERR("Interpolation 0 weight config[{}][{}] = {}", tok0.c_str(), tok1.c_str(), tok3.c_str());
										}
									}
								}
							}
						}
					}

					Globals::bounceInterpolationConfigMap[uniqueName] = newNPCBounceConfig;
					LOG_ERR("Bounce interpolation config loaded: {}", filename.c_str());
				}

			}
			Globals::actorBounceInterpolationConfigMap_lock.unlock();
		}
	}

	void loadSystemConfig()
	{
		std::string	runtimeDirectory = std::filesystem::current_path().string();

		if (!runtimeDirectory.empty())
		{
			std::string filepath = runtimeDirectory + "\\Data\\SKSE\\Plugins\\CBPCSystem.ini";

			std::ifstream file(filepath);

			if (!file.is_open())
			{
				std::ranges::transform(filepath, filepath.begin(), ::tolower);
				file.open(filepath);
			}

			if (file.is_open())
			{
				std::string line;
				std::string currentSetting;
				while (std::getline(file, line))
				{
					//trim(line);
					skipComments(line);
					trim(line);
					if (line.length() > 0)
					{
						if (line.substr(0, 1) == "[")
						{
							//newsetting
							currentSetting = line;
						}
						else
						{
							std::string variableName;

							int variableValue = GetConfigSettingsValue(line, variableName);
							if (variableName == "SkipFrames")
								Globals::collisionSkipFrames = variableValue;
							else if (variableName == "SkipFramesPelvis")
								Globals::collisionSkipFramesPelvis = variableValue;
							else if (variableName == "FpsCorrection")
								Globals::fpsCorrectionEnabled = variableValue;
							else if (variableName == "GridSize")
								Globals::gridsize = variableValue;
							else if (variableName == "AdjacencyValue")
								Globals::adjacencyValue = variableValue;
							else if (variableName == "ActorDistance")
							{
								float variableFloatValue = GetConfigSettingsFloatValue(line, variableName);
								Globals::actorDistance = variableFloatValue * variableFloatValue;
							}
							else if (variableName == "ActorBounceDistance")
							{
								float variableFloatValue = GetConfigSettingsFloatValue(line, variableName);
								Globals::actorBounceDistance = variableFloatValue * variableFloatValue;
							}
							else if (variableName == "ActorAngle")
							{
								Globals::actorAngle = variableValue;
							}
							else if (variableName == "UseCamera")
								Globals::useCamera = variableValue;
							else if (variableName == "Logging")
							{
								Globals::logging = variableValue;
							}
							else if (variableName == "InCombatActorCount")
							{
								Globals::inCombatActorCount = variableValue;
							}
							else if (variableName == "OutOfCombatActorCount")
							{
								Globals::outOfCombatActorCount = variableValue;
							}
						}
					}
				}

				LOG_ERR("System Config file is loaded successfully.");
				return;
			}
		}

		LOG_ERR("System Config file is not loaded.");
	}

	//// Converts the lower bits of a FormID to a full FormID depending on plugin type
	//uint32_t GetFullFormID(const ModInfo* modInfo, uint32_t formLower)
	//{
	//	return !modInfo->IsLight() ? uint32_t(modInfo->modIndex) << 24 | (formLower & 0xFFFFFF) : 0xFE000000 | (uint32_t(modInfo->lightIndex) << 12) | (formLower & 0xFFF);
	//}

	void GameLoad()
	{
		Globals::dialogueMenuOpen.store(false);
		Globals::raceSexMenuClosed.store(false);

		const auto dataHandler = RE::TESDataHandler::GetSingleton();

		auto lookupKeyword = [&](RE::FormID formID, std::string_view plugin) -> RE::BGSKeyword* {
			return dataHandler->LookupForm<RE::BGSKeyword>(formID, plugin);
		};

		Globals::KeywordArmorLight = lookupKeyword(Globals::KeywordArmorLightFormId, "Skyrim.esm");
		Globals::KeywordArmorHeavy = lookupKeyword(Globals::KeywordArmorHeavyFormId, "Skyrim.esm");
		Globals::KeywordArmorClothing = lookupKeyword(Globals::KeywordArmorClothingFormId, "Skyrim.esm");
		Globals::KeywordActorTypeNPC = lookupKeyword(Globals::KeywordActorTypeNPCFormId, "Skyrim.esm");

		if (dataHandler)
		{
			const auto* dawnguardMod = dataHandler->LookupModByName("Dawnguard.esm");

			if (dawnguardMod && !dawnguardMod->IsLight())
			{
				Globals::VampireLordBeastRaceFormId = dawnguardMod->GetFormID(0x0283A);
			}
		}
	}

	void loadMasterConfig()
	{
		std::string	runtimeDirectory = std::filesystem::current_path().string();

		if (!runtimeDirectory.empty())
		{
			Globals::configMap.clear();
			Globals::nodeConditionsMap.clear();
			Globals::affectedBones.clear();

			std::string filepath = runtimeDirectory + "\\Data\\SKSE\\Plugins\\CBPCMasterConfig.txt";

			std::ifstream file(filepath);

			if (!file.is_open())
			{
				std::ranges::transform(filepath, filepath.begin(), ::tolower);
				file.open(filepath);
			}

			if (file.is_open())
			{
				std::string line;
				std::string currentSetting;
				bool isThereVaginaNode = false;
				bool isThereAnusNode = false;
				bool isChain = false;
				std::vector<std::string> affectedBonesList;
				while (std::getline(file, line))
				{
					//trim(line);
					skipComments(line);
					trim(line);
					if (line.length() > 0)
					{
						if (line.substr(0, 1) == "[")
						{
							//newsetting
							currentSetting = line;
						}
						else
						{
							if (currentSetting == "[Settings]")
							{
								std::string variableName;
								int variableValue = GetConfigSettingsValue(line, variableName);

								if (variableName == "TuningMode")
									Globals::tuningModeCollision = variableValue;
								else if (variableName == "MalePhysics")
									Globals::malePhysics = variableValue;
								else if (variableName == "NoJitterFixNodes")
								{
									std::string variableStrValue = GetConfigSettingsStringValue(line, variableName);
									Globals::noJitterFixNodesList = split(variableStrValue, ',');
								}
							}
							else if (currentSetting == "[ConfigMap]")
							{
								if (line.substr(0, 1) == "<")
								{
									isChain = true;
								}
								else if (line.substr(0, 1) == ">")
								{
									isChain = false;
									Globals::affectedBones.emplace_back(affectedBonesList);
									affectedBonesList.clear();
								}
								else
								{
									std::string variableName;
									std::string conditions;
									std::string variableValue = GetConfigSettings2StringValues(line, variableName, conditions);
									bool isFound = false;
									if (variableName.compare("NPC L Pussy02") == 0 || variableName.compare("NPC R Pussy02") == 0 || variableName.compare("VaginaB1") == 0 || variableName.compare("Clitoral1") == 0)
										isThereVaginaNode = true;
									if (variableName.compare("NPC LT Anus2") == 0 || variableName.compare("NPC RT Anus2") == 0 || variableName.compare("NPC LB Anus2") == 0 || variableName.compare("NPC RB Anus2") == 0)
										isThereAnusNode = true;

									if (variableName.compare("NPC Pelvis [Pelv]") == 0 && isThereVaginaNode)
										continue;
									if (variableName.compare("Anal") == 0 && isThereAnusNode)
										continue;
									for (int i = 0; i < Globals::affectedBones.size(); i++)
									{
										if (std::ranges::find(Globals::affectedBones.at(i), variableName) != Globals::affectedBones.at(i).end())
										{
											isFound = true;
										}
									}

									if (!isFound)
									{
										affectedBonesList.emplace_back(variableName);
										if (!isChain)
										{
											Globals::affectedBones.emplace_back(affectedBonesList);
											affectedBonesList.clear();
										}
									}

									//Remove nodes 
									std::vector<int> indicesToRemove;

									for (int i = 0; i < Globals::affectedBones.size(); i++)
									{
										bool removeCurrent = false;

										auto FindPelvisNode = std::ranges::find(Globals::affectedBones.at(i), "NPC Pelvis [Pelv]");
										if (FindPelvisNode != Globals::affectedBones.at(i).end() && isThereVaginaNode)
										{
											Globals::affectedBones.at(i).erase(FindPelvisNode);
											removeCurrent = true;
										}

										auto FindAnalNode = std::ranges::find(Globals::affectedBones.at(i), "Anal");
										if (FindAnalNode != Globals::affectedBones.at(i).end() && isThereAnusNode)
										{
											Globals::affectedBones.at(i).erase(FindAnalNode);
											removeCurrent = true;
										}

										if (removeCurrent && Globals::affectedBones.at(i).empty())
										{
											indicesToRemove.push_back(i);
										}
									}

									for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); ++it)
									{
										Globals::affectedBones.erase(Globals::affectedBones.begin() + *it);
									}
									//

									if (variableValue != "")
									{
										Globals::configMap[variableName] = variableValue;
										LOG("ConfigMap[{}] = {} = {}", variableName.c_str(), variableValue.c_str(), conditions.c_str());
									}
									if (conditions != "")
									{
										Globals::nodeConditionsMap[variableName] = ParseConditions(conditions);
									}
								}
							}
						}
					}
				}

				LOG_ERR("Master Config file is loaded successfully.");
			}

			std::string configPath = runtimeDirectory + "\\Data\\SKSE\\Plugins\\";

			auto configList = get_all_files_names_within_folder(configPath.c_str());

			for (int i = 0; i < configList.size(); i++)
			{
				std::string filename = configList.at(i);

				if (filename == "." || filename == "..")
					continue;

				if (stringStartsWith(filename, "cbpcmasterconfig") && filename != "CBPCMasterConfig.txt" && (stringEndsWith(filename, ".txt") || stringEndsWith(filename, ".ini")))
				{
					std::string msg = "File found: " + filename;
					LOG("{}", msg.c_str());

					std::string filepath = configPath;
					filepath.append(filename);

					std::ifstream file(filepath);

					if (file.is_open())
					{
						std::string line;
						std::string currentSetting;
						bool isChain = false;
						bool isThereVaginaNode = false;
						bool isThereAnusNode = false;
						std::vector<std::string> affectedBonesList;
						while (std::getline(file, line))
						{
							//trim(line);
							skipComments(line);
							trim(line);
							if (line.length() > 0)
							{
								if (line.substr(0, 1) == "[")
								{
									//newsetting
									currentSetting = line;
								}
								else
								{
									if (currentSetting == "[Settings]")
									{
										std::string variableName;
										int variableValue = GetConfigSettingsValue(line, variableName);

										if (variableName == "NoJitterFixNodes")
										{
											std::string variableStrValue = GetConfigSettingsStringValue(line, variableName);
											std::vector<std::string> newNodesList = split(variableStrValue, ',');
											Globals::noJitterFixNodesList.insert(std::end(Globals::noJitterFixNodesList), std::begin(newNodesList), std::end(newNodesList));
										}
									}
									else if (currentSetting == "[ConfigMap]")
									{
										if (line.substr(0, 1) == "<")
										{
											isChain = true;
										}
										else if (line.substr(0, 1) == ">")
										{
											isChain = false;
											Globals::affectedBones.emplace_back(affectedBonesList);
											affectedBonesList.clear();
										}
										else
										{
											std::string variableName;
											std::string conditions;
											std::string variableValue = GetConfigSettings2StringValues(line, variableName, conditions);
											bool isFound = false;
											if (variableName.compare("NPC L Pussy02") == 0 || variableName.compare("NPC R Pussy02") == 0 || variableName.compare("VaginaB1") == 0 || variableName.compare("Clitoral1") == 0)
												isThereVaginaNode = true;
											if (variableName.compare("NPC LT Anus2") == 0 || variableName.compare("NPC RT Anus2") == 0 || variableName.compare("NPC LB Anus2") == 0 || variableName.compare("NPC RB Anus2") == 0)
												isThereAnusNode = true;
											if (variableName.compare("NPC Pelvis [Pelv]") == 0 && isThereVaginaNode)
												continue;
											if (variableName.compare("Anal") == 0 && isThereAnusNode)
												continue;
											for (int d = 0; d < Globals::affectedBones.size(); d++)
											{
												if (std::ranges::find(Globals::affectedBones.at(d), variableName) != Globals::affectedBones.at(d).end())
												{
													isFound = true;
												}
											}

											if (!isFound)
											{
												affectedBonesList.emplace_back(variableName);
												if (!isChain)
												{
													Globals::affectedBones.emplace_back(affectedBonesList);
													affectedBonesList.clear();
												}
											}

											//Remove nodes 
											std::vector<int> indicesToRemove;

											for (int i = 0; i < Globals::affectedBones.size(); i++)
											{
												bool removeCurrent = false;

												auto FindPelvisNode = std::ranges::find(Globals::affectedBones.at(i), "NPC Pelvis [Pelv]");
												if (FindPelvisNode != Globals::affectedBones.at(i).end() && isThereVaginaNode)
												{
													Globals::affectedBones.at(i).erase(FindPelvisNode);
													removeCurrent = true;
												}

												auto FindAnalNode = std::ranges::find(Globals::affectedBones.at(i), "Anal");
												if (FindAnalNode != Globals::affectedBones.at(i).end() && isThereAnusNode)
												{
													Globals::affectedBones.at(i).erase(FindAnalNode);
													removeCurrent = true;
												}

												if (removeCurrent && Globals::affectedBones.at(i).empty())
												{
													indicesToRemove.push_back(i);
												}
											}

											for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); ++it)
											{
												Globals::affectedBones.erase(Globals::affectedBones.begin() + *it);
											}
											//

											if (variableValue != "")
											{
												Globals::configMap[variableName] = variableValue;
												LOG("ConfigMap[{}] = {} = {}", variableName.c_str(), variableValue.c_str(), conditions.c_str());
											}
											if (conditions != "")
											{
												Globals::nodeConditionsMap[variableName] = ParseConditions(conditions);
											}
										}
									}
								}
							}
						}

						LOG_ERR("Extra Master Config file {} is loaded successfully.", filename.c_str());
					}
				}
			}
		}
	}

	void loadCollisionConfig()
	{
		std::string	runtimeDirectory = std::filesystem::current_path().string();

		if (!runtimeDirectory.empty())
		{
			Globals::AffectedNodeLines.clear();
			Globals::ColliderNodeLines.clear();
			Globals::AffectedNodesList.clear();
			Globals::ColliderNodesList.clear();

			std::string filepath = runtimeDirectory + "\\Data\\SKSE\\Plugins\\CBPCollisionConfig.txt";

			std::ifstream file(filepath);

			if (!file.is_open())
			{
				std::ranges::transform(filepath, filepath.begin(), ::tolower);
				file.open(filepath);
			}

			if (file.is_open())
			{
				std::string line;
				std::string currentSetting;
				float scaleWeight = 1.0f;
				while (std::getline(file, line))
				{
					//trim(line);
					skipComments(line);
					trim(line);
					if (line.length() > 0)
					{
						if (line.substr(0, 1) == "[")
						{
							std::vector<std::string> maincurrentSetting = split(line, ":");
							if (maincurrentSetting.size() > 1)
							{
								currentSetting = maincurrentSetting[0];
								scaleWeight = strtof(maincurrentSetting[1].c_str(), 0);
								if (scaleWeight > 1.0f)
									scaleWeight = 1.0f;
								else if (scaleWeight < 0.0f)
									scaleWeight = 0.0f;
							}
							else
							{
								//newsetting
								currentSetting = line;
							}
						}
						else
						{
							if (currentSetting == "[ExtraOptions]")
							{
								std::string variableName;
								float variableValue = GetConfigSettingsFloatValue(line, variableName);
								if (variableName == "BellyBulge")
								{
									Globals::cbellybulge = variableValue;
								}
								else if (variableName == "BellyBulgeMax")
								{
									Globals::cbellybulgemax = variableValue;
								}
								else if (variableName == "BellyBulgePosLowest")
								{
									Globals::cbellybulgeposlowest = variableValue;
								}
								else if (variableName == "BellyBulgeNodes")
								{
									std::string variableStrValue = GetConfigSettingsStringValue(line, variableName);
									Globals::bellybulgenodesList = split(variableStrValue, ',');
								}
								else if (variableName == "BellyBulgeNodesBlackList")
								{
									std::string variableStrValue = GetConfigSettingsStringValue(line, variableName);
									Globals::bellybulgenodesBlackList = split(variableStrValue, ',');
								}
								else if (variableName == "VaginaOpeningLimit")
								{
									Globals::vaginaOpeningLimit = variableValue;
								}
								else if (variableName == "VaginaOpeningMultiplier")
								{
									Globals::vaginaOpeningMultiplier = variableValue;
								}
								else if (variableName == "AnusOpeningLimit")
								{
									Globals::anusOpeningLimit = variableValue;
								}
								else if (variableName == "AnusOpeningMultiplier")
								{
									Globals::anusOpeningMultiplier = variableValue;
								}
								else if (variableName == "BellyBulgeSpeed")
								{
									Globals::bellyBulgeSpeed = variableValue;
								}
							}
							else if (currentSetting == "[AffectedNodes]")
							{
								std::vector<std::string> splittedList = splitMultiNonEmpty(line, ",()");
								if (splittedList.size() > 0)
								{
									Globals::AffectedNodeLines.emplace_back(splittedList[0]);
									ConfigLine newConfigLine;
									newConfigLine.NodeName = splittedList[0];
									if (splittedList.size() > 1)
									{
										for (int s = 1; s < splittedList.size(); s++)
										{
											if (splittedList[s] == "@")
											{
												newConfigLine.IgnoreAllSelfColliders = true;
											}
											else if (stringStartsWith(splittedList[s], "@"))
											{
												newConfigLine.IgnoredSelfColliders.emplace_back(splittedList[s].substr(1));
											}
											else
											{
												newConfigLine.IgnoredColliders.emplace_back(splittedList[s]);
											}
										}
									}
									Globals::AffectedNodesList.push_back(newConfigLine);
								}
							}
							else if (currentSetting == "[ColliderNodes]")
							{
								Globals::ColliderNodeLines.emplace_back(line);
								ConfigLine newConfigLine;
								newConfigLine.NodeName = line;
								Globals::ColliderNodesList.push_back(newConfigLine);
							}
							else
							{
								Sphere newSphere; //type 0
								Capsule newCapsule; //type 1

								int type = 0;
								std::vector<std::string> LowHighWeight = split(line, '|');

								std::vector<std::string> PointSplitted;

								if (LowHighWeight.size() > 1)
									PointSplitted = split(LowHighWeight[0], '&');
								else
									PointSplitted = split(line, '&');

								if (PointSplitted.size() == 1)
								{
									ConfigLineSplitterSphere(line, newSphere);
									type = 0;
								}
								else if (PointSplitted.size() == 2)
								{
									ConfigLineSplitterCapsule(line, newCapsule);
									type = 1;
								}

								std::string trimmedSetting = gettrimmed(currentSetting);

								if (std::find(Globals::AffectedNodeLines.begin(), Globals::AffectedNodeLines.end(), trimmedSetting) != Globals::AffectedNodeLines.end())
								{
									for (int i = 0; i < Globals::AffectedNodesList.size(); i++)
									{
										if (Globals::AffectedNodesList[i].NodeName == trimmedSetting)
										{
											if (type == 0)
											{
												newSphere.NodeName = Globals::AffectedNodesList[i].NodeName;
												Globals::AffectedNodesList[i].CollisionSpheres.emplace_back(newSphere);
												Globals::AffectedNodesList[i].scaleWeight = scaleWeight;
											}
											else if (type == 1)
											{
												newCapsule.NodeName = Globals::AffectedNodesList[i].NodeName;
												Globals::AffectedNodesList[i].CollisionCapsules.emplace_back(newCapsule);
												Globals::AffectedNodesList[i].scaleWeight = scaleWeight;
											}
											break;
										}
									}
								}
								if (std::find(Globals::ColliderNodeLines.begin(), Globals::ColliderNodeLines.end(), trimmedSetting) != Globals::ColliderNodeLines.end())
								{
									for (int i = 0; i < Globals::ColliderNodesList.size(); i++)
									{
										if (Globals::ColliderNodesList[i].NodeName == trimmedSetting)
										{
											if (type == 0)
											{
												newSphere.NodeName = Globals::ColliderNodesList[i].NodeName;
												Globals::ColliderNodesList[i].CollisionSpheres.emplace_back(newSphere);
												Globals::ColliderNodesList[i].scaleWeight = scaleWeight;
											}
											else if (type == 1)
											{
												newCapsule.NodeName = Globals::ColliderNodesList[i].NodeName;
												Globals::ColliderNodesList[i].CollisionCapsules.emplace_back(newCapsule);
												Globals::ColliderNodesList[i].scaleWeight = scaleWeight;
											}
											break;
										}
									}
								}
							}
						}
					}
				}
			}
			LOG_ERR("Collision Config file is loaded successfully.");
			return;
		}

		LOG_ERR("Collision Config file is not loaded.");
		return;
	}

	void loadExtraCollisionConfig()
	{
		std::string	runtimeDirectory = std::filesystem::current_path().string();

		if (!runtimeDirectory.empty())
		{
			Globals::specificNPCConfigList.clear();

			std::string configPath = runtimeDirectory + "\\Data\\SKSE\\Plugins\\";

			auto configList = get_all_files_names_within_folder(configPath.c_str());

			for (int i = 0; i < configList.size(); i++)
			{
				std::string filename = configList.at(i);

				if (filename == "." || filename == "..")
					continue;

				if (stringStartsWith(filename, "cbpcollisionconfig") && filename != "CBPCollisionConfig.txt" && (stringEndsWith(filename, ".txt") || stringEndsWith(filename, ".ini")))
				{
					std::string msg = "File found: " + filename;
					LOG("{}", msg.c_str());

					std::string filepath = configPath;
					filepath.append(filename);

					SpecificNPCConfig newNPCConfig;

					std::ifstream file(filepath);

					if (file.is_open())
					{
						std::string line;
						std::string currentSetting;
						float scaleWeight = 0.0f;
						while (std::getline(file, line))
						{
							//trim(line);
							skipComments(line);
							trim(line);
							if (line.length() > 0)
							{
								if (line.substr(0, 1) == "[")
								{
									std::vector<std::string> maincurrentSetting = split(line, ":");
									if (maincurrentSetting.size() > 1)
									{
										currentSetting = maincurrentSetting[0];
										scaleWeight = strtof(maincurrentSetting[1].c_str(), 0);
										if (scaleWeight > 1.0f)
											scaleWeight = 1.0f;
										else if (scaleWeight < 0.0f)
											scaleWeight = 0.0f;
									}
									else
									{
										//newsetting
										currentSetting = line;
									}
								}
								else
								{
									if (currentSetting == "[Options]")
									{
										std::string variableName;
										std::string variableValue = GetConfigSettingsStringValue(line, variableName);
										if (variableName == "Conditions")
										{
											newNPCConfig.conditions = ParseConditions(variableValue);
										}
										else if (variableName == "Priority")
										{
											newNPCConfig.ConditionPriority = GetConfigSettingsValue(line, variableName);
										}
										else if (variableName == "Characters")
										{
											ConditionItem cItem;
											cItem.single = false;

											std::vector<std::string> charactersList = split(variableValue, ',');

											for (auto& characterName : charactersList)
											{
												ConditionItem oItem;
												oItem.single = true;
												oItem.str = characterName;
												oItem.Not = false;
												oItem.type = ConditionType::ActorName;
												cItem.OrItems.emplace_back(oItem);
											}
											newNPCConfig.conditions.AndItems.emplace_back(cItem);
										}
										else if (variableName == "Races")
										{
											ConditionItem cItem;
											cItem.single = false;

											std::vector<std::string> raceList = split(variableValue, ',');
											for (auto& raceName : raceList)
											{
												ConditionItem oItem;
												oItem.single = true;
												oItem.str = raceName;
												oItem.Not = false;
												oItem.type = ConditionType::IsRaceName;
												cItem.OrItems.emplace_back(oItem);
											}
											newNPCConfig.conditions.AndItems.emplace_back(cItem);
										}
									}
									else if (currentSetting == "[ExtraOptions]")
									{
										std::string variableName;
										float variableValue = GetConfigSettingsFloatValue(line, variableName);
										if (variableName == "BellyBulge")
										{
											newNPCConfig.cbellybulge = variableValue;
										}
										else if (variableName == "BellyBulgeMax")
										{
											newNPCConfig.cbellybulgemax = variableValue;
										}
										else if (variableName == "BellyBulgePosLowest")
										{
											newNPCConfig.cbellybulgeposlowest = variableValue;
										}
										else if (variableName == "BellyBulgeNodes")
										{
											std::string variableStrValue = GetConfigSettingsStringValue(line, variableName);
											newNPCConfig.bellybulgenodesList = split(variableStrValue, ',');
										}
										else if (variableName == "BellyBulgeNodesBlackList")
										{
											std::string variableStrValue = GetConfigSettingsStringValue(line, variableName);
											newNPCConfig.bellybulgenodesBlackList = split(variableStrValue, ',');
										}
										else if (variableName == "VaginaOpeningLimit")
										{
											newNPCConfig.vaginaOpeningLimit = variableValue;
										}
										else if (variableName == "VaginaOpeningMultiplier")
										{
											newNPCConfig.vaginaOpeningMultiplier = variableValue;
										}
										else if (variableName == "AnusOpeningLimit")
										{
											newNPCConfig.anusOpeningLimit = variableValue;
										}
										else if (variableName == "AnusOpeningMultiplier")
										{
											newNPCConfig.anusOpeningMultiplier = variableValue;
										}
										else if (variableName == "BellyBulgeSpeed")
										{
											newNPCConfig.bellyBulgeSpeed = variableValue;
										}
									}
									else if (currentSetting == "[AffectedNodes]")
									{
										std::vector<std::string> splittedList = splitMultiNonEmpty(line, ",()");
										if (splittedList.size() > 0)
										{
											newNPCConfig.AffectedNodeLines.emplace_back(splittedList[0]);
											ConfigLine newConfigLine;
											newConfigLine.NodeName = splittedList[0];
											if (splittedList.size() > 1)
											{
												for (int s = 1; s < splittedList.size(); s++)
												{
													if (splittedList[s] == "@")
													{
														newConfigLine.IgnoreAllSelfColliders = true;
													}
													else if (stringStartsWith(splittedList[s], "@"))
													{
														newConfigLine.IgnoredSelfColliders.emplace_back(splittedList[s].substr(1));
													}
													else
													{
														newConfigLine.IgnoredColliders.emplace_back(splittedList[s]);
													}
												}
											}
											newNPCConfig.AffectedNodesList.push_back(newConfigLine);
										}
									}
									else if (currentSetting == "[ColliderNodes]")
									{
										newNPCConfig.ColliderNodeLines.emplace_back(line);
										ConfigLine newConfigLine;
										newConfigLine.NodeName = line;
										newNPCConfig.ColliderNodesList.push_back(newConfigLine);
									}
									else
									{
										Sphere newSphere; //type 0
										Capsule newCapsule; //type 1

										int type = 0;
										std::vector<std::string> LowHighWeight = split(line, '|');

										std::vector<std::string> PointSplitted;

										if (LowHighWeight.size() == 1)
											PointSplitted = split(line, '&');
										else
											PointSplitted = split(LowHighWeight[0], '&');

										if (PointSplitted.size() == 1)
										{
											ConfigLineSplitterSphere(line, newSphere);
											type = 0;
										}
										else if (PointSplitted.size() == 2)
										{
											ConfigLineSplitterCapsule(line, newCapsule);
											type = 1;
										}

										std::string trimmedSetting = gettrimmed(currentSetting);

										if (std::find(newNPCConfig.AffectedNodeLines.begin(), newNPCConfig.AffectedNodeLines.end(), trimmedSetting) != newNPCConfig.AffectedNodeLines.end())
										{
											for (int i = 0; i < newNPCConfig.AffectedNodesList.size(); i++)
											{
												if (newNPCConfig.AffectedNodesList[i].NodeName == trimmedSetting)
												{
													newNPCConfig.AffectedNodesList[i].scaleWeight = scaleWeight;
													if (type == 0)
														newNPCConfig.AffectedNodesList[i].CollisionSpheres.emplace_back(newSphere);
													else if (type == 1)
														newNPCConfig.AffectedNodesList[i].CollisionCapsules.emplace_back(newCapsule);
													break;
												}
											}
										}
										if (std::find(newNPCConfig.ColliderNodeLines.begin(), newNPCConfig.ColliderNodeLines.end(), trimmedSetting) != newNPCConfig.ColliderNodeLines.end())
										{
											for (int i = 0; i < newNPCConfig.ColliderNodesList.size(); i++)
											{
												if (newNPCConfig.ColliderNodesList[i].NodeName == trimmedSetting)
												{
													newNPCConfig.ColliderNodesList[i].scaleWeight = scaleWeight;
													if (type == 0)
														newNPCConfig.ColliderNodesList[i].CollisionSpheres.emplace_back(newSphere);
													else if (type == 1)
														newNPCConfig.ColliderNodesList[i].CollisionCapsules.emplace_back(newCapsule);
													break;
												}
											}
										}
									}
								}
							}
						}
						if (newNPCConfig.conditions.AndItems.size() > 0)
						{
							Globals::specificNPCConfigList.emplace_back(newNPCConfig);
						}
					}
				}
			}

			if (Globals::specificNPCConfigList.size() > 1)
			{
				std::sort(Globals::specificNPCConfigList.begin(), Globals::specificNPCConfigList.end(), compareConfigs);
			}

			LOG_ERR("Specific collision config files(if any) are loaded successfully.");
			return;
		}

		LOG_ERR("Specific collision config files are not loaded.");
		return;
	}


	void loadCollisionInterpolationConfig()
	{
		std::string	runtimeDirectory = std::filesystem::current_path().string();

		if (!runtimeDirectory.empty())
		{
			Globals::actorCollisionInterpolationConfigMap_lock.lock();
			Globals::collisionInterpolationConfigMap.clear();

			std::string configPath = runtimeDirectory + "\\Data\\SKSE\\Plugins\\";

			auto configList = get_all_files_names_within_folder(configPath.c_str());

			for (int i = 0; i < configList.size(); i++)
			{
				std::string filename = configList.at(i);

				if (filename == "." || filename == "..")
					continue;

				if (stringStartsWith(filename, "cbpccollisioninterpolationconfig") && filename != "cbpccollisioninterpolationconfig.txt" && (stringEndsWith(filename, ".txt") || stringEndsWith(filename, ".ini")))
				{
					std::string msg = "File found: " + filename;
					LOG("{}", msg.c_str());

					std::string filepath = configPath;
					filepath.append(filename);

					CollisionInterpolationConfig newInterpolationConfig;
					std::string uniqueName = "";

					std::ifstream file(filepath);

					if (file.is_open())
					{
						std::string line;
						std::string currentSetting;
						float scaleWeight = 0.0f;
						while (std::getline(file, line))
						{
							//trim(line);
							skipComments(line);
							trim(line);
							if (line.length() > 0)
							{
								if (line.substr(0, 1) == "[")
								{
									std::vector<std::string> maincurrentSetting = split(line, ":");
									if (maincurrentSetting.size() > 1)
									{
										currentSetting = maincurrentSetting[0];
										scaleWeight = strtof(maincurrentSetting[1].c_str(), 0);
										if (scaleWeight > 1.0f)
											scaleWeight = 1.0f;
										else if (scaleWeight < 0.0f)
											scaleWeight = 0.0f;
									}
									else
									{
										//newsetting
										currentSetting = line;
									}
								}
								else
								{
									if (currentSetting == "[Settings]")
									{
										std::string variableName;
										std::string variableValue = GetConfigSettingsStringValue(line, variableName);

										if (variableName == "UniqueName")
										{
											std::ranges::transform(variableValue, variableValue.begin(), ::tolower);
											uniqueName = variableValue;
											LOG_ERR("Collision interpolation config: {}", uniqueName.c_str());
										}
									}
									else if (currentSetting == "[AffectedNodes]")
									{
										std::vector<std::string> splittedList = splitMultiNonEmpty(line, ",()");
										if (splittedList.size() > 0)
										{
											newInterpolationConfig.AffectedNodeLines.push_back(splittedList[0]);
											ConfigLine newConfigLine;
											newConfigLine.NodeName = splittedList[0];
											LOG_ERR("Interpolation affectedNode: {}", newConfigLine.NodeName.c_str());
											newInterpolationConfig.AffectedNodesList.push_back(newConfigLine);
										}
									}
									else
									{
										Sphere newSphere; //type 0
										Capsule newCapsule; //type 1

										int type = 0;
										std::vector<std::string> LowHighWeight = split(line, '|');

										std::vector<std::string> PointSplitted;

										if (LowHighWeight.size() == 1)
											PointSplitted = split(line, '&');
										else
											PointSplitted = split(LowHighWeight[0], '&');

										if (PointSplitted.size() == 1)
										{
											ConfigLineSplitterSphere(line, newSphere);
											type = 0;
										}
										else if (PointSplitted.size() == 2)
										{
											ConfigLineSplitterCapsule(line, newCapsule);
											type = 1;
										}

										std::string trimmedSetting = gettrimmed(currentSetting);

										if (std::find(newInterpolationConfig.AffectedNodeLines.begin(), newInterpolationConfig.AffectedNodeLines.end(), trimmedSetting) != newInterpolationConfig.AffectedNodeLines.end())
										{
											for (int i = 0; i < newInterpolationConfig.AffectedNodesList.size(); i++)
											{
												if (newInterpolationConfig.AffectedNodesList[i].NodeName == trimmedSetting)
												{
													newInterpolationConfig.AffectedNodesList[i].scaleWeight = scaleWeight;
													if (type == 0)
														newInterpolationConfig.AffectedNodesList[i].CollisionSpheres.emplace_back(newSphere);
													else if (type == 1)
														newInterpolationConfig.AffectedNodesList[i].CollisionCapsules.emplace_back(newCapsule);
													break;
												}
											}
										}
									}
								}
							}
						}
						if (newInterpolationConfig.AffectedNodesList.size() > 0 && uniqueName.empty() == false)
						{
							Globals::collisionInterpolationConfigMap[uniqueName] = newInterpolationConfig;
						}
					}
				}
			}
			Globals::actorCollisionInterpolationConfigMap_lock.unlock();

			LOG_ERR("Collision Interpolation config files(if any) are loaded successfully.");
			return;
		}

		LOG_ERR("Collision Interpolation config files are not loaded.");
		return;
	}


	void ConfigLineSplitterSphere(std::string& line, Sphere& newSphere)
	{
		std::vector<std::string> lowHighSplitted = split(line, '|');
		if (lowHighSplitted.size() == 1)
		{
			std::vector<std::string> splittedFloats = split(lowHighSplitted[0], ',');
			if (splittedFloats.size() > 0)
			{
				newSphere.offset0.x = strtof(splittedFloats[0].c_str(), 0);
				newSphere.offset100.x = newSphere.offset0.x;
			}
			if (splittedFloats.size() > 1)
			{
				newSphere.offset0.y = strtof(splittedFloats[1].c_str(), 0);
				newSphere.offset100.y = newSphere.offset0.y;
			}
			if (splittedFloats.size() > 2)
			{
				newSphere.offset0.z = strtof(splittedFloats[2].c_str(), 0);
				newSphere.offset100.z = newSphere.offset0.z;
			}
			if (splittedFloats.size() > 3)
			{
				newSphere.radius0 = strtof(splittedFloats[3].c_str(), 0);
				newSphere.radius100 = newSphere.radius0;
			}
		}
		else if (lowHighSplitted.size() > 1)
		{
			std::vector<std::string> splittedFloats = split(lowHighSplitted[0], ',');
			if (splittedFloats.size() > 0)
			{
				newSphere.offset0.x = strtof(splittedFloats[0].c_str(), 0);
			}
			if (splittedFloats.size() > 1)
			{
				newSphere.offset0.y = strtof(splittedFloats[1].c_str(), 0);
			}
			if (splittedFloats.size() > 2)
			{
				newSphere.offset0.z = strtof(splittedFloats[2].c_str(), 0);
			}
			if (splittedFloats.size() > 3)
			{
				newSphere.radius0 = strtof(splittedFloats[3].c_str(), 0);
			}

			splittedFloats = split(lowHighSplitted[1], ',');
			if (splittedFloats.size() > 0)
			{
				newSphere.offset100.x = strtof(splittedFloats[0].c_str(), 0);
			}
			if (splittedFloats.size() > 1)
			{
				newSphere.offset100.y = strtof(splittedFloats[1].c_str(), 0);
			}
			if (splittedFloats.size() > 2)
			{
				newSphere.offset100.z = strtof(splittedFloats[2].c_str(), 0);
			}
			if (splittedFloats.size() > 3)
			{
				newSphere.radius100 = strtof(splittedFloats[3].c_str(), 0);
			}
		}
	}

	void ConfigLineSplitterCapsule(std::string& line, Capsule& newCapsules)
	{
		std::vector<std::string> lowHighSplitted = split(line, '|');

		if (lowHighSplitted.size() == 1)
		{
			std::vector<std::string> EndPointSplitted = split(lowHighSplitted[0], '&');

			std::vector<std::string> splittedFloats = split(EndPointSplitted[0], ',');
			if (splittedFloats.size() > 0)
			{
				newCapsules.End1_offset0.x = strtof(splittedFloats[0].c_str(), 0);
				newCapsules.End1_offset100.x = newCapsules.End1_offset0.x;
			}
			if (splittedFloats.size() > 1)
			{
				newCapsules.End1_offset0.y = strtof(splittedFloats[1].c_str(), 0);
				newCapsules.End1_offset100.y = newCapsules.End1_offset0.y;
			}
			if (splittedFloats.size() > 2)
			{
				newCapsules.End1_offset0.z = strtof(splittedFloats[2].c_str(), 0);
				newCapsules.End1_offset100.z = newCapsules.End1_offset0.z;
			}
			if (splittedFloats.size() > 3)
			{
				newCapsules.End1_radius0 = strtof(splittedFloats[3].c_str(), 0);
				newCapsules.End1_radius100 = newCapsules.End1_radius0;
			}

			splittedFloats = split(EndPointSplitted[1], ',');
			if (splittedFloats.size() > 0)
			{
				newCapsules.End2_offset0.x = strtof(splittedFloats[0].c_str(), 0);
				newCapsules.End2_offset100.x = newCapsules.End2_offset0.x;
			}
			if (splittedFloats.size() > 1)
			{
				newCapsules.End2_offset0.y = strtof(splittedFloats[1].c_str(), 0);
				newCapsules.End2_offset100.y = newCapsules.End2_offset0.y;
			}
			if (splittedFloats.size() > 2)
			{
				newCapsules.End2_offset0.z = strtof(splittedFloats[2].c_str(), 0);
				newCapsules.End2_offset100.z = newCapsules.End2_offset0.z;
			}
			if (splittedFloats.size() > 3)
			{
				newCapsules.End2_radius0 = strtof(splittedFloats[3].c_str(), 0);
				newCapsules.End2_radius100 = newCapsules.End2_radius0;
			}
		}
		else if (lowHighSplitted.size() > 1)
		{
			std::vector<std::string> EndPointSplitted = split(lowHighSplitted[0], '&');

			std::vector<std::string> splittedFloats = split(EndPointSplitted[0], ',');
			if (splittedFloats.size() > 0)
			{
				newCapsules.End1_offset0.x = strtof(splittedFloats[0].c_str(), 0);
			}
			if (splittedFloats.size() > 1)
			{
				newCapsules.End1_offset0.y = strtof(splittedFloats[1].c_str(), 0);
			}
			if (splittedFloats.size() > 2)
			{
				newCapsules.End1_offset0.z = strtof(splittedFloats[2].c_str(), 0);
			}
			if (splittedFloats.size() > 3)
			{
				newCapsules.End1_radius0 = strtof(splittedFloats[3].c_str(), 0);
			}

			splittedFloats = split(EndPointSplitted[1], ',');
			if (splittedFloats.size() > 0)
			{
				newCapsules.End2_offset0.x = strtof(splittedFloats[0].c_str(), 0);
			}
			if (splittedFloats.size() > 1)
			{
				newCapsules.End2_offset0.y = strtof(splittedFloats[1].c_str(), 0);
			}
			if (splittedFloats.size() > 2)
			{
				newCapsules.End2_offset0.z = strtof(splittedFloats[2].c_str(), 0);
			}
			if (splittedFloats.size() > 3)
			{
				newCapsules.End2_radius0 = strtof(splittedFloats[3].c_str(), 0);
			}

			EndPointSplitted = split(lowHighSplitted[1], '&');

			splittedFloats = split(EndPointSplitted[0], ',');
			if (splittedFloats.size() > 0)
			{
				newCapsules.End1_offset100.x = strtof(splittedFloats[0].c_str(), 0);
			}
			if (splittedFloats.size() > 1)
			{
				newCapsules.End1_offset100.y = strtof(splittedFloats[1].c_str(), 0);
			}
			if (splittedFloats.size() > 2)
			{
				newCapsules.End1_offset100.z = strtof(splittedFloats[2].c_str(), 0);
			}
			if (splittedFloats.size() > 3)
			{
				newCapsules.End1_radius100 = strtof(splittedFloats[3].c_str(), 0);
			}

			splittedFloats = split(EndPointSplitted[1], ',');
			if (splittedFloats.size() > 0)
			{
				newCapsules.End2_offset100.x = strtof(splittedFloats[0].c_str(), 0);
			}
			if (splittedFloats.size() > 1)
			{
				newCapsules.End2_offset100.y = strtof(splittedFloats[1].c_str(), 0);
			}
			if (splittedFloats.size() > 2)
			{
				newCapsules.End2_offset100.z = strtof(splittedFloats[2].c_str(), 0);
			}
			if (splittedFloats.size() > 3)
			{
				newCapsules.End2_radius100 = strtof(splittedFloats[3].c_str(), 0);
			}
		}
	}

	int GetConfigSettingsValue(std::string line, std::string& variable)
	{
		int value = 0;
		std::vector<std::string> splittedLine = split(line, '=');
		variable = "";
		if (splittedLine.size() > 1)
		{
			variable = splittedLine[0];
			trim(variable);

			std::string valuestr = splittedLine[1];
			trim(valuestr);
			try
			{
				value = std::stoi(valuestr);
			}
			catch (...)
			{
				value = 0;
			}
		}

		return value;
	}

	float GetConfigSettingsFloatValue(std::string line, std::string& variable)
	{
		float value = 0;
		std::vector<std::string> splittedLine = split(line, '=');
		variable = "";
		if (splittedLine.size() > 1)
		{
			variable = splittedLine[0];
			trim(variable);

			std::string valuestr = splittedLine[1];
			trim(valuestr);
			value = strtof(valuestr.c_str(), 0);
		}

		return value;
	}

	std::string GetConfigSettingsStringValue(std::string line, std::string& variable)
	{
		std::string valuestr = "";
		std::vector<std::string> splittedLine = split(line, '=');
		variable = "";
		if (splittedLine.size() > 0)
		{
			variable = splittedLine[0];
			trim(variable);
		}

		if (splittedLine.size() > 1)
		{
			valuestr = splittedLine[1];
			trim(valuestr);
		}

		return valuestr;
	}

	std::string GetConfigSettings2StringValues(std::string line, std::string& variable, std::string& value2)
	{
		std::string valuestr = "";
		std::vector<std::string> splittedLine = split(line, '=');
		variable = "";
		if (splittedLine.size() > 0)
		{
			variable = splittedLine[0];
			trim(variable);
		}

		if (splittedLine.size() > 1)
		{
			valuestr = splittedLine[1];
			trim(valuestr);
		}

		if (splittedLine.size() > 2)
		{
			value2 = splittedLine[2];
			trim(value2);
		}
		return valuestr;
	}

	void printSpheresMessage(std::string message, std::vector<Sphere> spheres)
	{
		for (int i = 0; i < spheres.size(); i++)
		{
			message += " Spheres: ";
			message += std::to_string(spheres[i].offset0.x);
			message += ",";
			message += std::to_string(spheres[i].offset0.y);
			message += ",";
			message += std::to_string(spheres[i].offset0.z);
			message += ",";
			message += std::to_string(spheres[i].radius0);
			message += " | ";
			message += std::to_string(spheres[i].offset100.x);
			message += ",";
			message += std::to_string(spheres[i].offset100.y);
			message += ",";
			message += std::to_string(spheres[i].offset100.z);
			message += ",";
			message += std::to_string(spheres[i].radius100);
		}
		LOG("{}", message.c_str());
	}

	std::vector<std::string> ConfigLineVectorToStringVector(std::vector<ConfigLine> linesList)
	{
		std::vector<std::string> outVector;

		for (int i = 0; i < linesList.size(); i++)
		{
			std::string str = linesList[i].NodeName;
			trim(str);
			outVector.emplace_back(str);
		}
		return outVector;
	}

	bool ConditionCheck(RE::Actor* actor, ConditionItem& condition)
	{
		// Helper to reduce boilerplate: applies Not logic
		auto check = [&](bool result) -> bool {
			return condition.Not ? !result : result;
		};

		// Helper to get actor base as TESNPC
		auto getActorBase = [&]() -> RE::TESNPC* {
			return actor->GetActorBase();
		};

		switch (condition.type) {

		case ConditionType::ActorFormId:
			return check(actor->formID == condition.id);

		case ConditionType::ActorName:
		{
			std::string name = (actor->formID == 0x14)
				? "Player"
				: actor->GetDisplayFullName();
			return check(name == condition.str);
		}

		case ConditionType::ActorWeightGreaterThan:
		{
			const float weight = actor->GetWeight();
			return check(weight > static_cast<float>(condition.id));
		}

		case ConditionType::IsRaceFormId:
		{
			const auto* race = actor->GetRace();
			return race && check(race->formID == condition.id);
		}

		case ConditionType::IsRaceName:
		{
			const auto* race = actor->GetRace();
			if (!race) return false;
			std::string raceName = race->GetFullName();
			return check(raceName == condition.str);
		}

		case ConditionType::IsFemale:
			return check(!IsActorMale(actor));

		case ConditionType::IsMale:
			return check(IsActorMale(actor));

		case ConditionType::IsInFaction:
		{
			bool hasFaction = false;
			actor->VisitFactions([&](RE::TESFaction* faction, std::int8_t) -> bool {
				if (faction && faction->formID == condition.id) {
					hasFaction = true;
					return false; // stop iteration
				}
				return true; // continue
			});
			return check(hasFaction);
		}

		case ConditionType::RaceHasKeywordId:
		{
			const auto* race = actor->GetRace();
			if (!race) return false;
			const auto* keyword = RE::TESForm::LookupByID<RE::BGSKeyword>(condition.id);
			return keyword && check(race->HasKeyword(keyword));
		}

		case ConditionType::RaceHasKeywordName:
		{
			const auto* race = actor->GetRace();
			if (!race) return false;
			return check(race->HasKeywordString(RE::BSFixedString(condition.str.c_str())));
		}

		case ConditionType::HasKeywordId:
		{
			const auto* actorBase = getActorBase();
			if (!actorBase) return false;
			const auto* keyword = RE::TESForm::LookupByID<RE::BGSKeyword>(condition.id);
			return keyword && check(actorBase->HasKeyword(keyword));
		}

		case ConditionType::HasKeywordName:
		{
			const auto* actorBase = getActorBase();
			if (!actorBase) return false;
			return check(actorBase->HasKeywordString(RE::BSFixedString(condition.str.c_str())));
		}

		case ConditionType::IsActorBase:
		{
			const auto* actorBase = getActorBase();
			return actorBase && check(actorBase->formID == condition.id);
		}

		case ConditionType::IsPlayerTeammate:
			return check(actor->IsPlayerTeammate());

		case ConditionType::IsPlayer:
			return check(actor->formID == 0x14);

		case ConditionType::IsUnique:
		{
			const auto* actorBase = getActorBase();
			return actorBase && check(actorBase->IsUnique());
		}

		case ConditionType::IsVoiceType:
		{
			const auto* actorBase = getActorBase();
			return actorBase &&
				check(actorBase->voiceType != nullptr &&
					actorBase->voiceType->formID == condition.id);
		}

		case ConditionType::IsCombatStyle:
		{
			const auto* actorBase = getActorBase();
			return actorBase &&
				check(actorBase->combatStyle != nullptr &&
					actorBase->combatStyle->formID == condition.id);
		}

		case ConditionType::IsClass:
		{
			const auto* actorBase = getActorBase();
			return actorBase &&
				check(actorBase->npcClass != nullptr &&
					actorBase->npcClass->formID == condition.id);
		}

		default:
			return false;
		}
	}

	bool GetSpecificNPCConfigForActor(RE::Actor* actor, SpecificNPCConfig& snc)
	{
		if (actor != nullptr)
		{
			for (int i = 0; i < Globals::specificNPCConfigList.size(); i++)
			{
				bool correct = true;

				for (int j = 0; j < Globals::specificNPCConfigList.at(i).conditions.AndItems.size(); j++)
				{
					if (Globals::specificNPCConfigList.at(i).conditions.AndItems.at(j).single)
					{
						if (!ConditionCheck(actor, Globals::specificNPCConfigList.at(i).conditions.AndItems.at(j)))
						{
							correct = false;
							break;
						}
					}
					else
					{
						bool innerCorrect = false;
						for (int o = 0; o < Globals::specificNPCConfigList.at(i).conditions.AndItems.at(j).OrItems.size(); o++)
						{
							if (Globals::specificNPCConfigList.at(i).conditions.AndItems.at(j).OrItems.at(o).single)
							{
								if (ConditionCheck(actor, Globals::specificNPCConfigList.at(i).conditions.AndItems.at(j).OrItems.at(o)))
								{
									innerCorrect = true;
									break;
								}
							}
						}
						if (innerCorrect == false)
						{
							correct = false;
							break;
						}
					}
				}

				if (correct)
				{
					snc = Globals::specificNPCConfigList.at(i);
					return true;
				}
			}
		}

		return false;
	}

	bool GetSpecificNPCBounceConfigForActor(RE::Actor* actor, SpecificNPCBounceConfig& snbc)
	{
		if (actor != nullptr)
		{
			for (int i = 0; i < Globals::specificNPCBounceConfigList.size(); i++)
			{
				bool correct = true;

				for (int j = 0; j < Globals::specificNPCBounceConfigList.at(i).conditions.AndItems.size(); j++)
				{
					if (Globals::specificNPCBounceConfigList.at(i).conditions.AndItems.at(j).single)
					{
						if (!ConditionCheck(actor, Globals::specificNPCBounceConfigList.at(i).conditions.AndItems.at(j)))
						{
							correct = false;
							break;
						}
					}
					else
					{
						bool innerCorrect = false;
						for (int o = 0; o < Globals::specificNPCBounceConfigList.at(i).conditions.AndItems.at(j).OrItems.size(); o++)
						{
							if (Globals::specificNPCBounceConfigList.at(i).conditions.AndItems.at(j).OrItems.at(o).single)
							{
								if (ConditionCheck(actor, Globals::specificNPCBounceConfigList.at(i).conditions.AndItems.at(j).OrItems.at(o)))
								{
									innerCorrect = true;
									break;
								}
							}
						}
						if (innerCorrect == false)
						{
							correct = false;
							break;
						}
					}
				}

				if (correct)
				{
					snbc = Globals::specificNPCBounceConfigList.at(i);
					return true;
				}
			}
		}

		return false;
	}

	//If the config of that part is not set and just set to default, return false
	bool IsConfigActuallyAllocated(SpecificNPCBounceConfig snbc, std::string section)
	{
		return (snbc.config[section]["stiffness"] >= 0.001f) || (snbc.config0weight[section]["stiffness"] >= 0.001f) //Doesn't set physics config?
			|| (snbc.config[section]["stiffnessX"] >= 0.001f) || (snbc.config0weight[section]["stiffnessX"] >= 0.001f)
			|| (snbc.config[section]["stiffnessY"] >= 0.001f) || (snbc.config0weight[section]["stiffnessY"] >= 0.001f)
			|| (snbc.config[section]["stiffnessZ"] >= 0.001f) || (snbc.config0weight[section]["stiffnessZ"] >= 0.001f)
			|| (snbc.config[section]["stiffnessXRot"] >= 0.001f) || (snbc.config0weight[section]["stiffnessXRot"] >= 0.001f)
			|| (snbc.config[section]["stiffnessYRot"] >= 0.001f) || (snbc.config0weight[section]["stiffnessYRot"] >= 0.001f)
			|| (snbc.config[section]["stiffnessZRot"] >= 0.001f) || (snbc.config0weight[section]["stiffnessZRot"] >= 0.001f)
			|| (snbc.config[section]["stiffness2"] >= 0.001f) || (snbc.config0weight[section]["stiffness2"] >= 0.001f)
			|| (snbc.config[section]["stiffness2X"] >= 0.001f) || (snbc.config0weight[section]["stiffness2X"] >= 0.001f)
			|| (snbc.config[section]["stiffness2Y"] >= 0.001f) || (snbc.config0weight[section]["stiffness2Y"] >= 0.001f)
			|| (snbc.config[section]["stiffness2Z"] >= 0.001f) || (snbc.config0weight[section]["stiffness2Z"] >= 0.001f)
			|| (snbc.config[section]["stiffness2XRot"] >= 0.001f) || (snbc.config0weight[section]["stiffness2XRot"] >= 0.001f)
			|| (snbc.config[section]["stiffness2YRot"] >= 0.001f) || (snbc.config0weight[section]["stiffness2YRot"] >= 0.001f)
			|| (snbc.config[section]["stiffness2ZRot"] >= 0.001f) || (snbc.config0weight[section]["stiffness2ZRot"] >= 0.001f)
			|| (snbc.config[section]["damping"] >= 0.001f) || (snbc.config0weight[section]["damping"] >= 0.001f)
			|| (snbc.config[section]["dampingX"] >= 0.001f) || (snbc.config0weight[section]["dampingX"] >= 0.001f)
			|| (snbc.config[section]["dampingY"] >= 0.001f) || (snbc.config0weight[section]["dampingY"] >= 0.001f)
			|| (snbc.config[section]["dampingZ"] >= 0.001f) || (snbc.config0weight[section]["dampingZ"] >= 0.001f)
			|| (snbc.config[section]["dampingXRot"] >= 0.001f) || (snbc.config0weight[section]["dampingXRot"] >= 0.001f)
			|| (snbc.config[section]["dampingYRot"] >= 0.001f) || (snbc.config0weight[section]["dampingYRot"] >= 0.001f)
			|| (snbc.config[section]["dampingZRot"] >= 0.001f) || (snbc.config0weight[section]["dampingZRot"] >= 0.001f)
			|| (snbc.config[section]["collisionXmaxoffset"] >= 100.001f) || (snbc.config[section]["collisionXmaxoffset"] <= 99.999f) //Doesn't set collision config?
			|| (snbc.config0weight[section]["collisionXmaxoffset"] >= 100.001f) || (snbc.config0weight[section]["collisionXmaxoffset"] <= 99.999f)
			|| (snbc.config[section]["collisionXminoffset"] <= -100.001f) || (snbc.config[section]["collisionXminoffset"] >= -99.999f)
			|| (snbc.config0weight[section]["collisionXminoffset"] <= -100.001f) || (snbc.config0weight[section]["collisionXminoffset"] >= -99.999f)
			|| (snbc.config[section]["collisionYmaxoffset"] >= 100.001f) || (snbc.config[section]["collisionYmaxoffset"] <= 99.999f)
			|| (snbc.config0weight[section]["collisionYmaxoffset"] >= 100.001f) || (snbc.config0weight[section]["collisionYmaxoffset"] <= 99.999f)
			|| (snbc.config[section]["collisionYminoffset"] <= -100.001f) || (snbc.config[section]["collisionYminoffset"] >= -99.999f)
			|| (snbc.config0weight[section]["collisionYminoffset"] <= -100.001f) || (snbc.config0weight[section]["collisionYminoffset"] >= -99.999f)
			|| (snbc.config[section]["collisionZmaxoffset"] >= 100.001f) || (snbc.config[section]["collisionZmaxoffset"] <= 99.999f)
			|| (snbc.config0weight[section]["collisionZmaxoffset"] >= 100.001f) || (snbc.config0weight[section]["collisionZmaxoffset"] <= 99.999f)
			|| (snbc.config[section]["collisionZminoffset"] <= -100.001f) || (snbc.config[section]["collisionZminoffset"] >= -99.999f)
			|| (snbc.config0weight[section]["collisionZminoffset"] <= -100.001f) || (snbc.config0weight[section]["collisionZminoffset"] >= -99.999f)
			|| (snbc.config[section]["amplitude"] >= 0.001f) || (snbc.config0weight[section]["amplitude"] >= 0.001f);
	}

	bool CheckActorForConditions(RE::Actor* actor, Conditions& conditions)
	{
		bool correct = true;
		if (actor != nullptr)
		{
			for (int j = 0; j < conditions.AndItems.size(); j++)
			{
				if (conditions.AndItems.at(j).single)
				{
					if (!ConditionCheck(actor, conditions.AndItems.at(j)))
					{
						correct = false;
						break;
					}
				}
				else
				{
					bool innerCorrect = false;
					for (int o = 0; o < conditions.AndItems.at(j).OrItems.size(); o++)
					{
						if (conditions.AndItems.at(j).OrItems.at(o).single)
						{
							if (ConditionCheck(actor, conditions.AndItems.at(j).OrItems.at(o)))
							{
								innerCorrect = true;
								break;
							}
						}
					}
					if (innerCorrect == false)
					{
						correct = false;
						break;
					}
				}
			}
		}

		return correct;
	}

	uint8_t IsActorMale(RE::Actor* actor)
	{
		return actor->GetActorBase()->GetSex() == RE::SEX::kMale;
	}

	//Menu Stuff

	/*EventResult AllMenuEventHandler::ReceiveEvent(RE::MenuOpenCloseEvent* evn, EventDispatcher<RE::MenuOpenCloseEvent>* dispatcher)
	{
		if (evn->opening)
		{
			MenuOpened(evn->menuName.c_str());
		}
		else
		{
			MenuClosed(evn->menuName.c_str());
		}

		return EventResult::kEvent_Continue;
	}

	void MenuOpened(std::string name)
	{
		if (name == "Dialogue Menu")
		{
			Globals::dialogueMenuOpen.store(true);
		}
		else if (name == "RaceSex Menu")
		{
			Globals::raceSexMenuOpen.store(true);
		}
		else if (name == "Main Menu")
		{
			Globals::MainMenuOpen.store(true);
			Globals::ActorNodeStoppedPhysicsMap.clear();
		}
	}*/

	void MenuClosed(const std::string& name)
	{
		if (name == "Dialogue Menu")
		{
			Globals::dialogueMenuOpen.store(false);
		}
		else if (name == "RaceSex Menu")
		{
			Globals::raceSexMenuClosed.store(true);
			Globals::raceSexMenuOpen.store(false);
		}
	}



	void LoadPlayerCollisionEventConfig()
	{
		Globals::PlayerCollisionEventNodes.clear();
		std::string	runtimeDirectory = std::filesystem::current_path().string();

		if (!runtimeDirectory.empty())
		{
			std::string configPath = runtimeDirectory + "\\Data\\SKSE\\Plugins\\CBPCPlayerCollisionEventConfig.txt";

			std::ifstream file(configPath);
			std::string line;
			std::string currentSetting;
			while (std::getline(file, line))
			{
				trim(line);
				skipComments(line);
				trim(line);
				if (line.length() > 0)
				{
					if (line.substr(0, 1) == "[")
					{
						//newsetting
						currentSetting = line;
					}
					else
					{
						if (currentSetting == "[PlayerCollisionEventNodes]")
						{
							Globals::PlayerCollisionEventNodes.push_back(line);
						}
						else if (currentSetting == "[Settings]")
						{
							std::string variableName;
							float variableValue = GetConfigSettingsFloatValue(line, variableName);
							if (variableName == "MinimumCollisionDuration")
							{
								Globals::MinimumCollisionDurationForEvent = variableValue;
							}
						}
					}
				}
			}

			LOG_ERR("Player Collision Event Config file is loaded successfully.");
			return;
		}

		LOG_ERR("Player Collision Event Config file is not loaded.");
		return;
	}




}
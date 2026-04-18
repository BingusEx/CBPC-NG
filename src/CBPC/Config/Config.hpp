#pragma once
#include "CBPC/Globals.hpp"
#include "CBPC/Collision/Shapes.hpp"


namespace CBP {

	void loadConfig();
	void GameLoad();
	void Log(const int msgLogLevel, const char* fmt, ...);

	//Collision Stuf
	bool compareConfigs(const SpecificNPCConfig& config1, const SpecificNPCConfig& config2);
	bool compareBounceConfigs(const SpecificNPCBounceConfig& config1, const SpecificNPCBounceConfig& config2);

	bool GetSpecificNPCBounceConfigForActor(RE::Actor* actor, SpecificNPCBounceConfig& snbc);
	bool IsConfigActuallyAllocated(SpecificNPCBounceConfig snbc, std::string section); //If the config of that part is not set and just set to default, return false

	void MenuOpened(std::string name);
	void MenuClosed(const std::string& name);
	void loadCollisionConfig();
	void loadMasterConfig();
	void loadExtraCollisionConfig();
	void loadSystemConfig();
	void LoadPlayerCollisionEventConfig();
	void loadCollisionInterpolationConfig();
	void loadBounceInterpolationConfig();

	void ConfigLineSplitterSphere(std::string& line, Sphere& newSphere);
	void ConfigLineSplitterCapsule(std::string& line, Capsule& newCapsule);

	int GetConfigSettingsValue(std::string line, std::string& variable);
	std::string GetConfigSettingsStringValue(std::string line, std::string& variable);
	std::string GetConfigSettings2StringValues(std::string line, std::string& variable, std::string& value2);

	float GetConfigSettingsFloatValue(std::string line, std::string& variable);
	void printSpheresMessage(std::string message, std::vector<Sphere> spheres);

	std::vector<std::string> ConfigLineVectorToStringVector(std::vector<ConfigLine> linesList);

	//The basic unit is parallel processing, but some physics chain nodes need sequential loading

	bool GetSpecificNPCConfigForActor(RE::Actor* actor, SpecificNPCConfig& snc);
	uint8_t IsActorMale(RE::Actor* actor);
	bool ConditionCheck(RE::Actor* actor, ConditionItem& condition);
	bool CheckActorForConditions(RE::Actor* actor, Conditions& conditions);

	std::string GetActorNodeString(RE::Actor* actor, const RE::BSFixedString& nodeName);
	std::string GetFormIdNodeString(uint32_t id, const RE::BSFixedString& nodeName);


}



#pragma once
#include "CBPC/Defs.hpp"
#include "CBPC/SimObj.hpp"

namespace CBP {

	//* I've never seen such a messy codebase
	//* For now just throw all the externs and whatnot here.
	//* Ill sort through them later

	class Globals {

		public:

		//--------- ActorEntry
		static inline tbb::concurrent_vector<ActorEntry> actorEntries;
		static inline std::map<std::pair<uint32_t, std::string_view>, RE::NiPoint3> thingDefaultPosList;
		static inline std::map<std::pair<uint32_t, std::string_view>, RE::NiMatrix3> thingDefaultRotList;


		//---------- Thing
		static constexpr float IntervalTime60Tick = 1.0f / 60.0f;
		static inline float IntervalTimeTick = IntervalTime60Tick;
		static inline float IntervalTimeTickScale = 1.0f;


		//---------- SimObj
		// Note we don't ref count the nodes becasue it's ignored when the Actor is deleted, and calling Release after that can corrupt memory
		static inline const char* leftBreastName = "NPC L Breast";
		static inline const char* rightBreastName = "NPC R Breast";
		static inline const char* LBreast01Name = "L Breast01";
		static inline const char* LBreast02Name = "L Breast02";
		static inline const char* LBreast03Name = "L Breast03";
		static inline const char* RBreast01Name = "R Breast01";
		static inline const char* RBreast02Name = "R Breast02";
		static inline const char* RBreast03Name = "R Breast03";
		static inline const char* leftButtName = "NPC L Butt";
		static inline const char* rightButtName = "NPC R Butt";
		static inline const char* bellyName = "HDT Belly";
		static inline const char* bellyName2 = "NPC Belly";
		static inline const char* leftPussy = "NPC L Pussy02";
		static inline const char* rightPussy = "NPC R Pussy02";
		static inline const char* pelvis = "NPC Pelvis [Pelv]";
		static inline const char* anal = "Anal";

		static inline RE::BSFixedString spine1String = "NPC Spine1 [Spn1]";

		static inline std::vector<std::string> femaleSpecificBones = {
			leftBreastName,
			rightBreastName,
			LBreast01Name,
			LBreast02Name,
			LBreast03Name,
			RBreast01Name,
			RBreast02Name,
			RBreast03Name,
			leftButtName,
			rightButtName,
			bellyName,
			bellyName2,
			pelvis
		};

		//CollisionHub.h
		static inline long callCount;
		static inline PartitionMap partitions;

		
		//---------- Config
		static inline std::vector<std::string> noJitterFixNodesList = {
			"NPC Genitals01 [Gen01]",
			"NPC Genitals02 [Gen02]",
			"NPC Genitals03 [Gen03]",
			"NPC Genitals04 [Gen04]",
			"NPC Genitals05 [Gen05]",
			"NPC Genitals06 [Gen06]"
		};

		static inline std::vector<std::string> bellybulgenodesList;
		static inline std::vector<std::string> bellybulgenodesBlackList;
		static inline std::vector<SpecificNPCConfig> specificNPCConfigList;
		static inline std::vector<SpecificNPCBounceConfig> specificNPCBounceConfigList;
		static inline std::vector<std::string> AffectedNodeLines;
		static inline std::vector<std::string> ColliderNodeLines;

		static inline tbb::concurrent_vector<std::string> PlayerCollisionEventNodes;
		static inline tbb::concurrent_vector<ConfigLine> AffectedNodesList; //Nodes that can be collided with
		static inline tbb::concurrent_vector<ConfigLine> ColliderNodesList; //Nodes that can collide nodes

		static inline tbb::concurrent_unordered_map<std::string, std::string> configMap;
		static inline tbb::concurrent_unordered_map<std::string, bool> ActorNodeStoppedPhysicsMap;
		static inline tbb::concurrent_unordered_map<std::string, PlayerCollisionEvent> ActorNodePlayerCollisionEventMap;
		static inline tbb::concurrent_unordered_map<std::string, PlayerCollisionEvent> ActorNodePlayerGenitalCollisionEventMap;
		static inline tbb::concurrent_unordered_map<std::string, Conditions> nodeConditionsMap;
		static inline tbb::concurrent_unordered_map<std::string, CollisionInterpolationConfig> collisionInterpolationConfigMap;
		static inline tbb::concurrent_unordered_map<std::string, BounceInterpolationConfig> bounceInterpolationConfigMap;
		static inline tbb::concurrent_unordered_map<uint32_t, tbb::concurrent_unordered_map<std::string, int>> actorBounceInterpolationsList;
		static inline tbb::concurrent_unordered_map<uint32_t, tbb::concurrent_unordered_map<std::string, int>> actorCollisionInterpolationsList;
		static inline tbb::concurrent_unordered_map<uint32_t, bool> actorFormIdRefreshBounceSettingsMap;
		static inline tbb::concurrent_unordered_map<uint32_t, bool> actorFormIdRefreshCollisionSettingsMap;

		static inline config_t config;
		static inline config_t config0weight;

		static inline int32_t configReloadCount = 0;
		static inline int32_t collisionSkipFrames = 0;
		static inline int32_t collisionSkipFramesPelvis = 5;
		static inline int32_t gridsize = 25;
		static inline int32_t adjacencyValue = 5;
		static inline int32_t tuningModeCollision = 0;
		static inline int32_t malePhysics = 0;
		static inline int32_t actorAngle = 180;
		static inline int32_t inCombatActorCount = 10;
		static inline int32_t outOfCombatActorCount = 30;

		static inline int32_t logging = 0;
		static inline int32_t useCamera = 0;
		static inline int32_t fpsCorrectionEnabled = 0;

		static inline uint32_t VampireLordBeastRaceFormId = 0x0200283A;

		static inline float actorDistance = 4194304.0f;
		static inline float actorBounceDistance = 16777216.0f;
		static inline float cbellybulge = 2.0f;
		static inline float cbellybulgemax = 10.0f;
		static inline float cbellybulgeposlowest = -12.0f;

		static inline float vaginaOpeningLimit = 5.0f;
		static inline float vaginaOpeningMultiplier = 4.0f;
		static inline float anusOpeningLimit = 5.0f;
		static inline float anusOpeningMultiplier = 4.0f;
		static inline float bellyBulgeSpeed = 0.5;
		static inline float MinimumCollisionDurationForEvent = 0.3f;

		static inline std::atomic<bool> dialogueMenuOpen = false;
		static inline std::atomic<bool> raceSexMenuClosed = false;
		static inline std::atomic<bool> raceSexMenuOpen = false;
		static inline std::atomic<bool> MainMenuOpen = false;
		static inline std::atomic<bool> consoleConfigReload = false;
		static inline std::atomic<bool> modPaused = false;

		static inline RE::BSFixedString breastGravityReferenceBoneString = "NPC Spine2 [Spn2]";

		static inline RE::BSFixedString GroundReferenceBone = "NPC Root [Root]";
		static inline RE::BSFixedString HighheelReferenceBone = "NPC";

		static inline RE::BGSKeyword* KeywordArmorClothing;
		static inline RE::BGSKeyword* KeywordArmorLight;
		static inline RE::BGSKeyword* KeywordArmorHeavy;
		static inline RE::BGSKeyword* KeywordActorTypeNPC;

		static inline RE::BSFixedString KeywordNameAsNakedL = "CBPCAsNakedL";
		static inline RE::BSFixedString KeywordNameAsNakedR = "CBPCAsNakedR";
		static inline RE::BSFixedString KeywordNameAsClothingL = "CBPCAsClothingL";
		static inline RE::BSFixedString KeywordNameAsClothingR = "CBPCAsClothingR";
		static inline RE::BSFixedString KeywordNameAsLightL = "CBPCAsLightL";
		static inline RE::BSFixedString KeywordNameAsLightR = "CBPCAsLightR";
		static inline RE::BSFixedString KeywordNameAsHeavyL = "CBPCAsHeavyL";
		static inline RE::BSFixedString KeywordNameAsHeavyR = "CBPCAsHeavyR";
		static inline RE::BSFixedString KeywordNameNoPushUpL = "CBPCNoPushUpL";
		static inline RE::BSFixedString KeywordNameNoPushUpR = "CBPCNoPushUpR";

		static inline std::shared_mutex actorBounceInterpolationConfigMap_lock;
		static inline std::shared_mutex actorBounceInterpolationList_lock;
		static inline std::shared_mutex actorBounceRefreshMap_lock;

		static inline std::shared_mutex actorCollisionInterpolationConfigMap_lock;
		static inline std::shared_mutex actorCollisionInterpolationList_lock;
		static inline std::shared_mutex actorCollisionRefreshMap_lock;

		static inline std::string versionStr = "1.6.2";
		static inline uint32_t version = 0x010602;

		//Doesnt exist
		//static inline SKSE::EventDispatcher<SKSEModCallbackEvent>* g_modEventDispatcher;
		/*static inline EventDispatcher<RE::TESEquipEvent>* g_TESEquipEventDispatcher;
		static inline TESEquipEventHandler g_TESEquipEventHandler;
		static inline AllMenuEventHandler menuEvent;*/

		static inline bool debugtimelog = false;

		static inline std::vector<std::vector<std::string>> affectedBones;

		static inline uint32_t KeywordArmorLightFormId = 0x06BBD3;
		static inline uint32_t KeywordArmorHeavyFormId = 0x06BBD2;
		static inline uint32_t KeywordArmorClothingFormId = 0x06BBE8;
		static inline uint32_t KeywordActorTypeNPCFormId = 0x0013794;

		//SCAN
		static inline tbb::concurrent_unordered_map<uint32_t, SimObj> actors;

		static inline std::vector<uint32_t> notExteriorWorlds = { 0x69857, 0x1EE62, 0x20DCB, 0x1FAE2, 0x34240, 0x50015, 0x2C965, 0x29AB7, 0x4F838, 0x3A9D6, 0x243DE, 0xC97EB, 0xC350D, 0x1CDD3, 0x1CDD9, 0x21EDB, 0x1E49D, 0x2B101, 0x2A9D8, 0x20BFE };

		static inline std::atomic<RE::TESObjectCELL*> curCell = nullptr;
		static inline std::atomic<bool> curCellSpaceExterior = false;
		static inline std::atomic<uint32_t> curCellWorldspaceFormId = 0;

		static inline int frameCount = 0;
		static inline bool inCreatureForm = false;
		static inline float lastPlayerWeight = 50.0f;
		static inline int skipFramesScanCount = 0;

		static inline LARGE_INTEGER startingTime, endingTime, elapsedMicroseconds;
		static inline LARGE_INTEGER frequency;

		static inline bool firsttimeloginit = true;
		static inline LARGE_INTEGER totaltime;
		static inline int debugtimelog_framecount = 1;
		static inline int totalcallcount = 0;
		static inline int GameStepTimerCount = 0;
		static inline float GameStepTimerHap = 0.0f;

		static inline RE::BSFixedString leftPus = "NPC L Pussy02";
		static inline RE::BSFixedString rightPus = "NPC R Pussy02";
		static inline RE::BSFixedString backPus = "VaginaB1";
		static inline RE::BSFixedString frontPus = "Clitoral1";
		static inline RE::BSFixedString belly = "HDT Belly";
		static inline RE::BSFixedString spine1 = "NPC Spine1 [Spn1]";
		static inline RE::BSFixedString frontAnus = "NPC LT Anus2";
		static inline RE::BSFixedString backAnus = "NPC RT Anus2";
		static inline RE::BSFixedString leftAnus = "NPC LB Anus2";
		static inline RE::BSFixedString rightAnus = "NPC RB Anus2";

		/*static inline EventDispatcher<RE::TESEquipEvent>* g_TESEquipEventDispatcher;
		static inline TESEquipEventHandler g_TESEquipEventHandler;*/
	};


	//Free functions found in headers

	inline void RefreshNode(RE::NiAVObject* a_node) {
		if (a_node == nullptr || a_node->name == nullptr) {
			return;
		}

		if (std::ranges::find(Globals::noJitterFixNodesList, a_node->name.c_str()) != 
			Globals::noJitterFixNodesList.end()) {
			return;
		}

		{
			RE::NiUpdateData updateData;
			updateData.time = 0.0f;

			a_node->UpdateWorldData(&updateData);
		}
	}

	inline RE::NiPoint3 GetPointFromPercentage(RE::NiPoint3 lowWeight, RE::NiPoint3 highWeight, float weight){
		return ((highWeight - lowWeight) * (weight * 0.01f)) + lowWeight;
	}

	inline RE::NiPoint3 GetVectorFromCollision(RE::NiPoint3 col, RE::NiPoint3 thing, float Scalar, float currentDistance) {
		return (thing - col) / currentDistance * Scalar; // normalized vector * scalar
	}

	inline float distance(RE::NiPoint3 po1, RE::NiPoint3 po2){

		float x = po1.x - po2.x;
		float y = po1.y - po2.y;
		float z = po1.z - po2.z;
		float result = std::sqrt(x * x + y * y + z * z);

		return result;
	}

	inline uint32_t fastTrunc(float x) {
		if (x >= 0.0f) return static_cast<uint32_t>(std::ceil(x));
		return static_cast<uint32_t>(std::floor(x));
	}

	inline uint32_t generateId(uint8_t v1, uint8_t v2, uint8_t v3){
		uint32_t id = v1 | (static_cast<uint32_t>(v2) << 8) | (static_cast<uint32_t>(v3) << 16);
		return id;
	}

	inline uint32_t CreateHashId(float x, float y, float z, uint32_t gridSize, uint32_t actordistance){
		return generateId(fastTrunc((x + actordistance) / gridSize), fastTrunc((y + actordistance) / gridSize), fastTrunc((z + actordistance) / gridSize));
	}


	//------ UTILITY.H

	// trim from start (in place)
	inline void ltrim(std::string& s){
		Util::Text::TrimL(s);
	}

	// trim from end (in place)
	inline void rtrim(std::string& s){
		Util::Text::TrimR(s);
	}

	// trim from both ends (in place)
	inline void trim(std::string& s){
		ltrim(s);
		rtrim(s);
	}

	// trim from start (copying)
	inline std::string ltrim_copy(std::string s){
		ltrim(s);
		return s;
	}

	// trim from end (copying)
	inline std::string rtrim_copy(std::string s){
		rtrim(s);
		return s;
	}

	// trim from both ends (copying)
	inline std::string trim_copy(std::string s){
		trim(s);
		return s;
	}

	inline std::vector<std::string> split(const std::string& s, char delimiter) {
		
		std::string str = trim_copy(s);
		std::vector<std::string> tokens;
		if (!str.empty()) {
			std::string token;
			std::istringstream tokenStream(str);
			while (std::getline(tokenStream, token, delimiter)) {
				trim(token);
				tokens.emplace_back(token);
			}
		}
		return tokens;
	}

	inline std::vector<std::string> splitNonEmpty(const std::string& s, char delimiter) {
		std::string str = trim_copy(s);
		std::vector<std::string> tokens;
		if (!str.empty()) {
			std::string token;
			std::istringstream tokenStream(str);
			while (std::getline(tokenStream, token, delimiter))
			{
				trim(token);
				if (token.size() > 0)
					tokens.emplace_back(token);
			}
		}
		return tokens;
	}

	inline std::vector<std::string> splitMulti(const std::string& s, const std::string& delimiters) {
		std::string str = trim_copy(s);

		std::vector<std::string> tokens;
		std::stringstream stringStream(str);
		std::string line;
		while (std::getline(stringStream, line)) {
			std::size_t prev = 0, pos;
			while ((pos = line.find_first_of(delimiters, prev)) != std::string::npos) {
				if (pos > prev) {
					std::string token = line.substr(prev, pos - prev);
					trim(token);
					tokens.emplace_back(token);
				}

				prev = pos + 1;
			}
			if (prev < line.length()) {
				std::string token = line.substr(prev, std::string::npos);
				trim(token);
				tokens.emplace_back(token);
			}
		}
		return tokens;
	}

	inline std::vector<std::string> splitMultiNonEmpty(const std::string& s, const std::string& delimiters) {
		std::string str = trim_copy(s);

		std::vector<std::string> tokens;
		std::stringstream stringStream(str);
		std::string line;
		while (std::getline(stringStream, line)) {
			std::size_t prev = 0, pos;
			while ((pos = line.find_first_of(delimiters, prev)) != std::string::npos) {
				if (pos > prev) {
					std::string token = line.substr(prev, pos - prev);
					trim(token);
					if (token.size() > 0)
						tokens.emplace_back(token);
				}

				prev = pos + 1;
			}
			if (prev < line.length()) {
				std::string token = line.substr(prev, std::string::npos);
				trim(token);
				if (token.size() > 0)
					tokens.emplace_back(token);
			}
		}
		return tokens;
	}

	inline std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
		size_t pos_start = 0, pos_end, delim_len = delimiter.length();
		std::vector<std::string> res;

		while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
			std::string token = s.substr(pos_start, pos_end - pos_start);
			trim(token);
			pos_start = pos_end + delim_len;
			res.emplace_back(token);
		}

		std::string lasttoken = s.substr(pos_start);
		trim(lasttoken);
		res.emplace_back(lasttoken);
		return res;
	}

	inline bool Contains(const std::string& str, const std::string& ministr) {
		if (str.find(ministr) != std::string::npos) {
			return true;
		}
		return false;
	}

	inline bool ContainsNoCase(std::string str, std::string ministr) {
		std::ranges::transform(str, str.begin(), ::tolower);
		std::ranges::transform(ministr, ministr.begin(), ::tolower);

		if (str.find(ministr) != std::string::npos) {
			return true;
		}
		return false;
	}

	inline void skipComments(std::string& str) {
		auto pos = str.find("#");
		if (pos != std::string::npos) {
			str.erase(pos);
		}
	}

	inline std::string gettrimmed(const std::string& s) {
		std::string temp = s;
		if (temp.front() == '[' && temp.back() == ']') {
			temp.erase(0, 1);
			temp.pop_back();
		}
		return temp;
	}

	inline void NormalizeNiPoint(RE::NiPoint3& collisionVector, float low, float high) {
		if (collisionVector.x < low)       collisionVector.x = low;
		else if (collisionVector.x > high) collisionVector.x = high;

		if (collisionVector.y < low)       collisionVector.y = low;
		else if (collisionVector.y > high) collisionVector.y = high;

		if (collisionVector.z < low)       collisionVector.z = low;
		else if (collisionVector.z > high) collisionVector.z = high;
	}

	inline float clamp(float val, float min, float max) {
		if (val < min) return min;
		if (val > max) return max;
		return val;
	}

	inline bool CompareNiPoints(RE::NiPoint3 collisionVector, RE::NiPoint3 point) {
		return fabsf(collisionVector.x - point.x) < 0.000001f && fabsf(collisionVector.y - point.y) < 0.000001f && fabsf(collisionVector.z - point.z) < 0.000001f;
	}

	inline RE::BSFixedString ReturnUsableString(const std::string& str) {
		RE::BSFixedString fs(str.c_str());
		return fs;
	}

	inline float distanceNoSqrt2D(RE::NiPoint3 po1, RE::NiPoint3 po2) {

		float x = po1.x - po2.x;
		float y = po1.y - po2.y;
		float result = x * x + y * y;

		return result;
	}

	inline float distanceNoSqrt(RE::NiPoint3 po1, RE::NiPoint3 po2) {

		float x = po1.x - po2.x;
		float y = po1.y - po2.y;
		float z = po1.z - po2.z;
		float result = x * x + y * y + z * z;

		return result;
	}

	inline void GetAttitudeAndHeadingFromTwoPoints(RE::NiPoint3 source, RE::NiPoint3 target, float& attitude, float& heading) {
		RE::NiPoint3 vector = target - source;

		const float sqr = vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
		if (sqr != 0) {
			vector = vector * (1.0f / sqrtf(sqr));
			attitude = std::atan2(vector.z, sqrtf(vector.y * vector.y + vector.x * vector.x)) * -1;
			heading = std::atan2(vector.x, vector.y);
		}
		else {
			attitude = 0;
			heading = 0;
		}
	}

	inline float AngleDifference(float angle1, float angle2) {
		if ((angle1 < -90 && angle2 > 90) || (angle2 < -90 && angle1 > 90)) {
			return (180 - abs(angle1)) + (180 - abs(angle2));
		}
		if ((angle1 > -90 && 0 >= angle1 && angle2 < 90 && 0 <= angle2) || (angle2 > -90 && 0 >= angle2 && angle1 < 90 && 0 <= angle1)) {
			return abs(angle1) + abs(angle2);
		}
		return abs(angle1 - angle2);
	}

	inline bool IsValidModIndex(uint32_t modIndex) {
		return modIndex > 0 && modIndex != 0xFF;
	}

	// get base formID (without mod index)
	inline uint32_t GetBaseFormID(uint32_t formId){
		return formId & 0x00FFFFFF;
	}

	inline std::vector<std::string> get_all_files_names_within_folder(const std::string& folder) {
		std::vector<std::string> names;

		namespace fs = std::filesystem;
		std::error_code ec;

		fs::directory_iterator it(folder, ec);
		fs::directory_iterator end;

		if (ec) return names;

		for (; it != end; it.increment(ec)) {
			if (ec) break;

			if (it->is_regular_file(ec) && !ec) {
				names.emplace_back(it->path().filename().string());
			}
		}

		return names;
	}

	inline bool stringStartsWith(std::string str, const std::string& prefix) {
		// convert string to back to lower case
		std::ranges::for_each(str, [](char& c){
			c = ::tolower(c);
		});
		// std::string::find returns 0 if toMatch is found at starting
		if (str.find(prefix) == 0) return true;
		return false;
	}

	inline bool stringEndsWith(std::string str, std::string const& suffix) {
		std::ranges::for_each(str, [](char& c){
			c = ::tolower(c);
		});
		if (str.length() >= suffix.length()){
			return (0 == str.compare(str.length() - suffix.length(), suffix.length(), suffix));
		}
		return false;
		
	}

	inline double GetPercentageValue(double number1, double number2, float perc) {
		if (perc == 100) return number2;
		if (perc == 0)   return number1;
		return number1 + ((number2 - number1) * (perc * 0.01f));
		
	}

	inline float GetPercentageValue(float number1, float number2, float perc) {
		if (perc == 100) return number2;
		if (perc == 0) return number1;
		return number1 + ((number2 - number1) * (perc * 0.01f));
		
	}

	inline double vlibGetSetting(const char* name) {
		RE::Setting* setting = RE::GetINISetting(name);
		if (!setting) return -1;
		return setting->GetFloat();
	}

	inline float magnitude(RE::NiPoint3 p) {
		return sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
	}

	inline float magnitude2d(RE::NiPoint3 p) {
		return sqrtf(p.x * p.x + p.y * p.y);
	}

	inline float magnitudePwr2(RE::NiPoint3 p) {
		return p.x * p.x + p.y * p.y + p.z * p.z;
	}

	inline RE::NiPoint3 crossProduct(RE::NiPoint3 A, RE::NiPoint3 B) {
		return RE::NiPoint3(A.y * B.z - A.z * B.y, A.z * B.x - A.x * B.z, A.x * B.y - A.y * B.x);
	}

	inline uint32_t getHex(const std::string& hexstr) {
		return static_cast<uint32_t>(std::strtoul(hexstr.c_str(), 0, 16));
	}

	template <typename I> std::string num2hex(I w, size_t hex_len = sizeof(I) << 1) {
		static const char* digits = "0123456789ABCDEF";
		std::string rc(hex_len, '0');
		for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
			rc[i] = digits[(w >> j) & 0x0f];
		return rc;
	}

	inline bool isWantSlot(RE::TESObjectARMO* thisArmor, uint32_t wantSlot) {
		uint32_t slot = (thisArmor) ? thisArmor->bipedModelData.bipedObjectSlots.underlying() : 0;
		return (slot == wantSlot);
	}

	inline std::string spaces(int n) {
		auto s = std::string(n, ' ');
		return s;
	}

	inline void dumpTransform(RE::NiTransform t) {
		Util::Text::Print("%8.2f %8.2f %8.2f", t.rotate.entry[0][0], t.rotate.entry[0][1], t.rotate.entry[0][2]);
		Util::Text::Print("%8.2f %8.2f %8.2f", t.rotate.entry[1][0], t.rotate.entry[1][1], t.rotate.entry[1][2]);
		Util::Text::Print("%8.2f %8.2f %8.2f", t.rotate.entry[2][0], t.rotate.entry[2][1], t.rotate.entry[2][2]);

		Util::Text::Print("%8.2f %8.2f %8.2f", t.translate.x, t.translate.y, t.translate.z);
		Util::Text::Print("%8.2f", t.scale);
	}

	/*
		bool visitObjects(NiAVObject  *parent, std::function<bool(NiAVObject*, int)> functor, int depth = 0) {
			if (!parent) return false;
			NiNode * node = parent->GetAsNiNode();
			if (node) {
				if (functor(parent, depth))
					return true;

				for (uint32_t i = 0; i < node->m_children.m_emptyRunStart; i++) {
					NiAVObject * object = node->m_children.m_data[i];
					if (object) {
						if (visitObjects(object, functor, depth + 1))
							return true;
					}
				}
			}
			else if (functor(parent, depth))
				return true;

			return false;
		}
	*/

	inline float GetFrameIntervalTimeTick() {
		static float* gSecondsSinceLastFrame_WorldTime = reinterpret_cast<float*>(REL::RelocationID(523660, 410199).address());
		return *gSecondsSinceLastFrame_WorldTime;
	}

	inline bool IsActorValid(RE::Actor* a_actor) {
		if (a_actor->GetFormFlags() & RE::TESForm::RecordFlags::kDeleted) return false;
		if (a_actor && a_actor->loadedData && a_actor->loadedData->data3D) return true;
		return false;
	}

	inline bool ActorInCombat(RE::Actor* a_actor) {
		return a_actor->IsInCombat();
	}

	inline std::string GetActorNodeString(RE::Actor* actor, const RE::BSFixedString& nodeName) {
		return num2hex(actor->formID, 8) + ":" + nodeName.c_str();
	}

	inline std::string GetFormIdNodeString(uint32_t id, const RE::BSFixedString& nodeName) {
		return num2hex(id, 8) + ":" + nodeName.c_str();
	}

}

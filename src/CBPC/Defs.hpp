#pragma once
#include "CBPC/Collision/Shapes.hpp"

namespace CBP {
	class Collision;

	//ColisionHub.h
	struct Partition {
		tbb::concurrent_vector<Collision> partitionCollisions;
	};

	typedef tbb::concurrent_unordered_map<uint32_t, Partition> PartitionMap;
	typedef tbb::concurrent_unordered_map<std::string, float> configEntry_t;
	typedef tbb::concurrent_unordered_map<std::string, configEntry_t> config_t;

	struct ActorEntry {
		uint32_t id;
		RE::Actor* actor;
		uint8_t sex;
		float actorDistSqr;
		bool collisionsEnabled = true;
	};

	struct PlayerCollisionEvent {
		bool collisionInThisCycle = false;
		float durationFilled = 0.0f;
		float totalDurationFilled = 0.0f;
	};

	enum eLogLevels {
		LOGLEVEL_ERR = 0,
		LOGLEVEL_WARN,
		LOGLEVEL_INFO,
	};

	enum ConditionType {
		IsRaceFormId,
		IsRaceName,
		ActorFormId,
		ActorName,
		ActorWeightGreaterThan,
		IsMale,
		IsFemale,
		IsPlayer,
		IsInFaction,
		HasKeywordId,
		HasKeywordName,
		RaceHasKeywordId,
		RaceHasKeywordName,
		IsActorBase,
		IsPlayerTeammate,
		IsUnique,
		IsVoiceType,
		IsCombatStyle,
		IsClass
	};

	struct ConfigLine {
		std::vector<Sphere> CollisionSpheres;
		std::vector<Capsule> CollisionCapsules;
		std::string NodeName;
		std::vector<std::string> IgnoredColliders;
		std::vector<std::string> IgnoredSelfColliders;
		bool IgnoreAllSelfColliders = false;
		float scaleWeight = 1.0f;
	};

	struct ConditionItem {
		//multiple items
		std::vector<ConditionItem> OrItems;

		//single item
		bool single = true;
		bool Not = false;
		ConditionType type;
		uint32_t id;
		std::string str;
	};

	struct Conditions {
		std::vector<ConditionItem> AndItems;
	};

	struct SpecificNPCConfig {
		Conditions conditions;
		int ConditionPriority = 50;

		std::vector<std::string> AffectedNodeLines;
		std::vector<std::string> ColliderNodeLines;

		tbb::concurrent_vector<ConfigLine> AffectedNodesList;
		tbb::concurrent_vector<ConfigLine> ColliderNodesList;

		float cbellybulge;
		float cbellybulgemax;
		float cbellybulgeposlowest;

		std::vector<std::string> bellybulgenodesList;
		std::vector<std::string> bellybulgenodesBlackList;

		float bellyBulgeSpeed = 1.0f;

		float vaginaOpeningLimit = 5.0f;
		float vaginaOpeningMultiplier = 4.0f;
		float anusOpeningLimit = 5.0f;
		float anusOpeningMultiplier = 4.0f;
	};

	struct SpecificNPCBounceConfig {
		Conditions conditions;
		int ConditionPriority = 50;

		config_t config;
		config_t config0weight;
	};

	struct CollisionInterpolationConfig {
		tbb::concurrent_vector<std::string> AffectedNodeLines;
		tbb::concurrent_vector<ConfigLine> AffectedNodesList;
	};

	struct BounceInterpolationConfig {
		config_t config;
		config_t config0weight;
	};

	/*class TESEquipEventHandler : public RE::BSTEventSink <RE::TESEquipEvent> {
		public:
		virtual	RE::EventResult ReceiveEvent(RE::TESEquipEvent* evn, RE::EventDispatcher<RE::TESEquipEvent>* dispatcher);
	};

	class AllMenuEventHandler : public RE::BSTEventSink <RE::MenuOpenCloseEvent> {
		public:
		virtual EventResult	ReceiveEvent(RE::MenuOpenCloseEvent* evn, EventDispatcher<RE::MenuOpenCloseEvent>* dispatcher);
	};*/
}
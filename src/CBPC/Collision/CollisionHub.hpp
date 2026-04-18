#pragma once
#include "CBPC/Globals.hpp"

namespace CBP {
	
	bool CreateActorColliders(RE::Actor* actor, tbb::concurrent_unordered_map<std::string, Collision>& actorCollidersList);
	void UpdateColliderPositions(tbb::concurrent_unordered_map<std::string, Collision>& colliderList, tbb::concurrent_unordered_map<std::string, RE::NiPoint3> NodeCollisionSyncList);
	int GetHashIdFromPos(RE::NiPoint3 pos);
	std::vector<int> GetHashIdsFromPos(RE::NiPoint3 pos, float radiusplus);
	bool CheckPelvisArmor(RE::Actor* actor);

}
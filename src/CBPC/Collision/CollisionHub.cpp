#include "CBPC/Collision/CollisionHub.hpp"
#include "CBPC/Config/Config.hpp"

namespace CBP {

	bool CreateActorColliders(RE::Actor* actor, tbb::concurrent_unordered_map<std::string, Collision>& actorCollidersList) {
		
		bool GroundCollisionEnabled = false;
		RE::NiNode* mostInterestingRoot;

		if (actor && actor->loadedData && actor->loadedData->data3D) {
			mostInterestingRoot = netimmerse_cast<RE::NiNode*>(actor->loadedData->data3D.get());
		}
		else return false;

		float npcWeight = 50.0f;
		float actorBaseScale = 1.0f;
		if (actor) {
			npcWeight = actor->GetWeight();
			actorBaseScale = actor->GetBaseHeight();
		}

		tbb::concurrent_vector<ConfigLine>* ColliderNodesListPtr;

		SpecificNPCConfig snc;

		if (actor) {
			if (GetSpecificNPCConfigForActor(actor, snc)) {
				ColliderNodesListPtr = &(snc.ColliderNodesList);
			}
			else {
				ColliderNodesListPtr = &Globals::ColliderNodesList;
			}
		}
		else {
			ColliderNodesListPtr = &Globals::ColliderNodesList;
		}

		std::shared_mutex CH_read_lock;

		tbb::parallel_for(static_cast<size_t>(0), ColliderNodesListPtr->size(), [&](size_t j) {
			//detecting NPC Root [Root] node for ground collision
			if (ColliderNodesListPtr->at(j).NodeName.compare(Globals::GroundReferenceBone.data()) == 0) {
				GroundCollisionEnabled = true;
			}
			else {
				RE::BSFixedString fs = ReturnUsableString(ColliderNodesListPtr->at(j).NodeName);
				CH_read_lock.lock();
				RE::NiAVObject* node = mostInterestingRoot->GetObjectByName(fs.data());
				CH_read_lock.unlock();
				if (node) {
					Collision newCol = Collision(node, ColliderNodesListPtr->at(j).CollisionSpheres, ColliderNodesListPtr->at(j).CollisionCapsules, npcWeight);
					newCol.colliderActor = actor;
					newCol.colliderNodeName = fs.data();
					newCol.scaleWeight = ColliderNodesListPtr->at(j).scaleWeight;
					newCol.actorBaseScale = actorBaseScale;

					actorCollidersList.insert(std::make_pair(ColliderNodesListPtr->at(j).NodeName, newCol));
				}
			}
		});
		return GroundCollisionEnabled;
	}

	//Unfortunately this doesn't work.
	bool CheckPelvisArmor(RE::Actor* actor) {
		return false;
		//return papyrusActor::GetWornForm(actor, 49) != NULL && papyrusActor::GetWornForm(actor, 52) != NULL && papyrusActor::GetWornForm(actor, 53) != NULL && papyrusActor::GetWornForm(actor, 54) != NULL && papyrusActor::GetWornForm(actor, 56) != NULL && papyrusActor::GetWornForm(actor, 58) != NULL;
	}

	void UpdateColliderPositions(tbb::concurrent_unordered_map<std::string, Collision>& colliderList, tbb::concurrent_unordered_map<std::string, RE::NiPoint3> NodeCollisionSyncList) {
		tbb::parallel_for_each(colliderList.begin(), colliderList.end(), [&](auto& collider) {
			RE::NiPoint3 VirtualOffset = {};

			if (NodeCollisionSyncList.find(collider.second.colliderNodeName) != NodeCollisionSyncList.end()) {
				VirtualOffset = NodeCollisionSyncList[collider.second.colliderNodeName];
			}

			float colliderNodescale = 1.0f - ((1.0f - (collider.second.CollisionObject->world.scale / collider.second.actorBaseScale)) * collider.second.scaleWeight);

			for (int j = 0; j < collider.second.collisionSpheres.size(); j++) {
				collider.second.collisionSpheres[j].offset100 = collider.second.collisionSpheres[j].offset0 * collider.second.actorBaseScale * colliderNodescale;
				collider.second.collisionSpheres[j].worldPos = collider.second.CollisionObject->world.translate + (collider.second.CollisionObject->world.rotate * collider.second.collisionSpheres[j].offset100) + VirtualOffset;
				collider.second.collisionSpheres[j].radius100 = collider.second.collisionSpheres[j].radius0 * collider.second.actorBaseScale * colliderNodescale;
				collider.second.collisionSpheres[j].radius100pwr2 = collider.second.collisionSpheres[j].radius100 * collider.second.collisionSpheres[j].radius100;
			}

			for (int k = 0; k < collider.second.collisionCapsules.size(); k++) {
				collider.second.collisionCapsules[k].End1_offset100 = collider.second.collisionCapsules[k].End1_offset0 * collider.second.actorBaseScale * colliderNodescale;
				collider.second.collisionCapsules[k].End1_worldPos = collider.second.CollisionObject->world.translate + (collider.second.CollisionObject->world.rotate * collider.second.collisionCapsules[k].End1_offset100) + VirtualOffset;
				collider.second.collisionCapsules[k].End1_radius100 = collider.second.collisionCapsules[k].End1_radius0 * collider.second.actorBaseScale * colliderNodescale;
				collider.second.collisionCapsules[k].End1_radius100pwr2 = collider.second.collisionCapsules[k].End1_radius100 * collider.second.collisionCapsules[k].End1_radius100;
				collider.second.collisionCapsules[k].End2_offset100 = collider.second.collisionCapsules[k].End2_offset0 * collider.second.actorBaseScale * colliderNodescale;
				collider.second.collisionCapsules[k].End2_worldPos = collider.second.CollisionObject->world.translate + (collider.second.CollisionObject->world.rotate * collider.second.collisionCapsules[k].End2_offset100) + VirtualOffset;
				collider.second.collisionCapsules[k].End2_radius100 = collider.second.collisionCapsules[k].End2_radius0 * collider.second.actorBaseScale * colliderNodescale;
				collider.second.collisionCapsules[k].End2_radius100pwr2 = collider.second.collisionCapsules[k].End2_radius100 * collider.second.collisionCapsules[k].End2_radius100;
			}
		});
	}

	std::vector<int> GetHashIdsFromPos(RE::NiPoint3 pos, float radiusplus) {

		std::vector<int> hashIdList;

		int hashId = CreateHashId(pos.x, pos.y, pos.z, Globals::gridsize, Globals::actorDistance);
		LOG_INFO("hashId={}", hashId);
		if (hashId >= 0) hashIdList.emplace_back(hashId);

		bool xPlus = false;
		bool xMinus = false;
		bool yPlus = false;
		bool yMinus = false;
		bool zPlus = false;
		bool zMinus = false;

		hashId = CreateHashId(pos.x + radiusplus, pos.y, pos.z, Globals::gridsize, Globals::actorDistance);
		LOG_INFO("hashId={}", hashId);
		if (hashId >= 0) {
			if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
				xPlus = true;
				hashIdList.emplace_back(hashId);
			}
		}

		hashId = CreateHashId(pos.x - radiusplus, pos.y, pos.z, Globals::gridsize, Globals::actorDistance);
		LOG_INFO("hashId={}", hashId);
		if (hashId >= 0) {
			if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
				xMinus = true;
				hashIdList.emplace_back(hashId);
			}
		}

		hashId = CreateHashId(pos.x, pos.y + radiusplus, pos.z, Globals::gridsize, Globals::actorDistance);
		LOG_INFO("hashId={}", hashId);
		if (hashId >= 0) {
			if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
				yPlus = true;
				hashIdList.emplace_back(hashId);
			}
		}

		hashId = CreateHashId(pos.x, pos.y - radiusplus, pos.z, Globals::gridsize, Globals::actorDistance);
		LOG_INFO("hashId={}", hashId);
		if (hashId >= 0) {
			if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
				yMinus = true;
				hashIdList.emplace_back(hashId);
			}
		}

		hashId = CreateHashId(pos.x, pos.y, pos.z + radiusplus, Globals::gridsize, Globals::actorDistance);
		LOG_INFO("hashId={}", hashId);
		if (hashId >= 0) {
			if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
				zPlus = true;
				hashIdList.emplace_back(hashId);
			}
		}

		hashId = CreateHashId(pos.x, pos.y, pos.z - radiusplus, Globals::gridsize, Globals::actorDistance);
		LOG_INFO("hashId={}", hashId);
		if (hashId >= 0) {
			if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
				zMinus = true;
				hashIdList.emplace_back(hashId);
			}
		}

		if (xPlus && yPlus) {
			hashId = CreateHashId(pos.x + radiusplus, pos.y + radiusplus, pos.z, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end())
				{
					hashIdList.emplace_back(hashId);
				}
			}

			if (xPlus && yPlus && zPlus) {
				hashId = CreateHashId(pos.x + radiusplus, pos.y + radiusplus, pos.z + radiusplus, Globals::gridsize, Globals::actorDistance);
				LOG_INFO("hashId={}", hashId);
				if (hashId >= 0)
				{
					if (std::ranges::find(hashIdList, hashId) == hashIdList.end())
					{
						hashIdList.emplace_back(hashId);
					}
				}
			}
			else if (xPlus && yPlus && zMinus) {
				hashId = CreateHashId(pos.x + radiusplus, pos.y + radiusplus, pos.z - radiusplus, Globals::gridsize, Globals::actorDistance);
				LOG_INFO("hashId={}", hashId);
				if (hashId >= 0)
				{
					if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
						hashIdList.emplace_back(hashId);
					}
				}
			}
		}
		else if (xMinus && yPlus) {
			hashId = CreateHashId(pos.x - radiusplus, pos.y + radiusplus, pos.z, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
					hashIdList.emplace_back(hashId);
				}
			}

			if (xMinus && yPlus && zMinus)
			{
				hashId = CreateHashId(pos.x - radiusplus, pos.y + radiusplus, pos.z - radiusplus, Globals::gridsize, Globals::actorDistance);
				LOG_INFO("hashId={}", hashId);
				if (hashId >= 0) {
					if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
						hashIdList.emplace_back(hashId);
					}
				}
			}
			else if (xMinus && yPlus && zPlus)
			{
				hashId = CreateHashId(pos.x - radiusplus, pos.y + radiusplus, pos.z + radiusplus, Globals::gridsize, Globals::actorDistance);
				LOG_INFO("hashId={}", hashId);
				if (hashId >= 0)
				{
					if (std::ranges::find(hashIdList, hashId) == hashIdList.end())
					{
						hashIdList.emplace_back(hashId);
					}
				}
			}
		}
		else if (xPlus && yMinus) {
			hashId = CreateHashId(pos.x + radiusplus, pos.y - radiusplus, pos.z, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
					hashIdList.emplace_back(hashId);
				}
			}

			if (xPlus && yMinus && zMinus) {
				hashId = CreateHashId(pos.x + radiusplus, pos.y - radiusplus, pos.z - radiusplus, Globals::gridsize, Globals::actorDistance);
				LOG_INFO("hashId={}", hashId);
				if (hashId >= 0) {
					if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
						hashIdList.emplace_back(hashId);
					}
				}
			}
			else if (xPlus && yMinus && zPlus) {
				hashId = CreateHashId(pos.x + radiusplus, pos.y - radiusplus, pos.z + radiusplus, Globals::gridsize, Globals::actorDistance);
				LOG_INFO("hashId={}", hashId);
				if (hashId >= 0) {
					if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
						hashIdList.emplace_back(hashId);
					}
				}
			}
		}
		else if (xMinus && yMinus) {
			hashId = CreateHashId(pos.x - radiusplus, pos.y - radiusplus, pos.z, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
					hashIdList.emplace_back(hashId);
				}
			}

			if (xMinus && yMinus && zMinus) {
				hashId = CreateHashId(pos.x - radiusplus, pos.y - radiusplus, pos.z - radiusplus, Globals::gridsize, Globals::actorDistance);
				LOG_INFO("hashId={}", hashId);
				if (hashId >= 0) {
					if (std::ranges::find(hashIdList, hashId) == hashIdList.end())
					{
						hashIdList.emplace_back(hashId);
					}
				}
			}
			else if (xMinus && yMinus && zPlus) {
				hashId = CreateHashId(pos.x - radiusplus, pos.y - radiusplus, pos.z + radiusplus, Globals::gridsize, Globals::actorDistance);
				LOG_INFO("hashId={}", hashId);
				if (hashId >= 0) {
					if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
						hashIdList.emplace_back(hashId);
					}
				}
			}
		}

		if (xPlus && zPlus) {
			hashId = CreateHashId(pos.x + radiusplus, pos.y, pos.z + radiusplus, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
					hashIdList.emplace_back(hashId);
				}
			}
		}
		else if (xMinus && zPlus) {
			hashId = CreateHashId(pos.x - radiusplus, pos.y, pos.z + radiusplus, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
					hashIdList.emplace_back(hashId);
				}
			}
		}
		else if (xPlus && zMinus) {
			hashId = CreateHashId(pos.x + radiusplus, pos.y, pos.z - radiusplus, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
					hashIdList.emplace_back(hashId);
				}
			}
		}
		else if (xMinus && zMinus) {
			hashId = CreateHashId(pos.x - radiusplus, pos.y, pos.z - radiusplus, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
					hashIdList.emplace_back(hashId);
				}
			}
		}

		if (yPlus && zPlus) {
			hashId = CreateHashId(pos.x, pos.y + radiusplus, pos.z + radiusplus, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
					hashIdList.emplace_back(hashId);
				}
			}
		}
		else if (yMinus && zPlus) {
			hashId = CreateHashId(pos.x, pos.y - radiusplus, pos.z + radiusplus, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end())
				{
					hashIdList.emplace_back(hashId);
				}
			}
		}
		else if (yPlus && zMinus) {
			hashId = CreateHashId(pos.x, pos.y + radiusplus, pos.z - radiusplus, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
					hashIdList.emplace_back(hashId);
				}
			}
		}
		else if (yMinus && zMinus) {
			hashId = CreateHashId(pos.x, pos.y - radiusplus, pos.z - radiusplus, Globals::gridsize, Globals::actorDistance);
			LOG_INFO("hashId={}", hashId);
			if (hashId >= 0) {
				if (std::ranges::find(hashIdList, hashId) == hashIdList.end()) {
					hashIdList.emplace_back(hashId);
				}
			}
		}

		return hashIdList;
	}

	int GetHashIdFromPos(RE::NiPoint3 pos) {
		int hashId = CreateHashId(pos.x, pos.y, pos.z, Globals::gridsize, Globals::actorDistance);
		if (hashId >= 0) return hashId;
		return -1;
	}
}

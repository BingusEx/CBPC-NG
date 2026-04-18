#include "CBPC/SimObj.hpp"
#include "CBPC/Globals.hpp"
#include "CBPC/Thing.hpp"

#include "Collision/CollisionHub.hpp"

#include "Config/Config.hpp"

namespace CBP {

	SimObj::~SimObj() {}

	SimObj::SimObj() {}

	SimObj::SimObj(RE::Actor* a_owner) : m_Things() {
		ID = a_owner->formID;
		m_OwnerActor = a_owner;
	}

	bool SimObj::Bind(RE::Actor* a_actor, bool a_isMale) {
		RE::LOADED_REF_DATA* loadedState = a_actor->loadedData;
		if (loadedState && loadedState->data3D.get()) {
			Bound = true;

			m_Things.clear();
			m_ActorColliders.clear();
			const std::string actorRace = a_actor->GetRace()->fullName.c_str();

			std::shared_mutex obj_read_lock;

			tbb::parallel_for(static_cast<size_t>(0), Globals::affectedBones.size(), [&](size_t i) {
				std::string firstcs;
				bool isfirstbone = true;

				std::unordered_map<std::string, Thing> thingmsg;

				for (int j = 0; j < Globals::affectedBones.at(i).size(); j++)
				{
					if (a_isMale && Globals::malePhysics == 0) {
						if (std::ranges::find(Globals::femaleSpecificBones, Globals::affectedBones.at(i).at(j)) != Globals::femaleSpecificBones.end()
							|| ContainsNoCase(Globals::affectedBones.at(i).at(j), "breast")
							|| ContainsNoCase(Globals::affectedBones.at(i).at(j), "thigh")
							|| ContainsNoCase(Globals::affectedBones.at(i).at(j), "calf")) {
							continue;
						}
					}

					if ((Globals::nodeConditionsMap.find(Globals::affectedBones.at(i).at(j)) != Globals::nodeConditionsMap.end()) &&
						!CheckActorForConditions(a_actor, Globals::nodeConditionsMap[Globals::affectedBones.at(i).at(j)])) {
						continue;
					}

					std::string cs = ReturnUsableString(Globals::affectedBones.at(i).at(j)).c_str();
					RE::BSFixedString tmp = cs;

					obj_read_lock.lock();
					RE::NiAVObject* bone = loadedState->data3D->GetObjectByName(cs.data());
					obj_read_lock.unlock();

					if (bone)
					{
						thingmsg.emplace(cs, Thing(a_actor, bone, tmp));

						if (isfirstbone)
						{
							firstcs = cs;
							isfirstbone = false;
						}
					}
				}

				if (!isfirstbone) {
					auto [it, inserted] = m_Things.emplace(firstcs, tbb::concurrent_unordered_map<std::string, Thing>{});
					for (auto& [key, thing] : thingmsg) {
						it->second.emplace(key, std::move(thing));
					}
				}
			});
			m_EnableGroundCollisions = CreateActorColliders(a_actor, m_ActorColliders);
			UpdateConfig(a_actor);

			return true;
		}
		return false;
	}

	bool SimObj::UpdateConfig(RE::Actor* a_owner) {
		float actorWeight = 50;

		if (a_owner) {
			actorWeight = a_owner->GetWeight();
			tbb::parallel_for_each(m_Things.begin(), m_Things.end(), [&](auto& t) {

				for (auto& tt : t.second) {
					tt.second.actorWeight = actorWeight;
					std::string& section = Globals::configMap[tt.first];

					SpecificNPCBounceConfig snbc;
					//If the config of that part is not set and just set to default, run to no condition
					if (GetSpecificNPCBounceConfigForActor(a_owner, snbc) && IsConfigActuallyAllocated(snbc, section)){
						tt.second.updateConfig(a_owner, snbc.config[section], snbc.config0weight[section], section);
					}
					else {
						tt.second.updateConfig(a_owner, Globals::config[section], Globals::config0weight[section], section);
					}
					tt.second.GroundCollisionEnabled = m_EnableGroundCollisions;
				}
			});
		}
		return true;
	}

	void SimObj::Update(RE::Actor* a_owner, bool a_CollisionsEnabled, bool a_IsFemale) {

		if (!Bound) return;
		
		if (!RE::PlayerCharacter::GetSingleton() || !(RE::PlayerCharacter::GetSingleton())->loadedData || !RE::PlayerCharacter::GetSingleton()->loadedData->data3D.get()) {
			return;
		}
		if (m_EnableGroundCollisions && a_CollisionsEnabled)
		{
			if (a_owner->loadedData && a_owner->loadedData->data3D.get())
			{
				RE::NiAVObject* groundobj = a_owner->loadedData->data3D->GetObjectByName(Globals::GroundReferenceBone);
				if (groundobj)
				{
					m_GroundPos = groundobj->world.translate.z; //Get ground by NPC Root [Root] node
					RE::NiAVObject* highheelobj = a_owner->loadedData->data3D->GetObjectByName(Globals::HighheelReferenceBone.data());
					if (highheelobj) {
						m_GroundPos -= highheelobj->local.translate.z; //Get highheel offset by NPC node
					}
				}
			}
		}

		std::shared_mutex thing_ReadNode_lock, thing_SetNode_lock;
		tbb::parallel_for_each(m_Things.begin(), m_Things.end(), [&](auto& t) {
			//The basic unit is parallel processing, but some physics chain nodes need sequential loading
			for (auto& tt : t.second) {
				bool isStopPhysics = false;
				std::string actorNodeString = GetActorNodeString(a_owner, tt.first);
				if (Globals::ActorNodeStoppedPhysicsMap.find(actorNodeString) != Globals::ActorNodeStoppedPhysicsMap.end())
					isStopPhysics = Globals::ActorNodeStoppedPhysicsMap[actorNodeString];

				if (!isStopPhysics) {
					Globals::ActorNodePlayerCollisionEventMap[actorNodeString].collisionInThisCycle = false;
					Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].collisionInThisCycle = false;

					tt.second.ActorCollisionsEnabled = a_CollisionsEnabled;
					if (strcmp(tt.first.c_str(), Globals::pelvis) == 0)
					{
						tt.second.updatePelvis(a_owner, thing_ReadNode_lock, thing_SetNode_lock);
					}
					else if (strcmp(tt.first.c_str(), Globals::anal) == 0)
					{
						tt.second.updateAnal(a_owner, thing_ReadNode_lock, thing_SetNode_lock);
					}
					else
					{
						tt.second.groundPos = m_GroundPos;
						tt.second.update(a_owner, thing_ReadNode_lock, thing_SetNode_lock);
						if (tt.second.VirtualCollisionEnabled)
						{
							m_NodeCollisionSync[tt.first] = tt.second.collisionSync;
						}
					}

					if (Globals::ActorNodePlayerCollisionEventMap[actorNodeString].collisionInThisCycle == true) //Node got player collision in this cycle
					{
						Globals::ActorNodePlayerCollisionEventMap[actorNodeString].durationFilled += Globals::IntervalTimeTick;
						Globals::ActorNodePlayerCollisionEventMap[actorNodeString].totalDurationFilled += Globals::IntervalTimeTick;

						if (Globals::ActorNodePlayerCollisionEventMap[actorNodeString].durationFilled >= Globals::MinimumCollisionDurationForEvent)
						{
							//TODO DISABLED FOR NOW
							/*if (g_modEventDispatcher != NULL) {
								RE::BSFixedString modEventName(a_IsFemale ? "CBPCPlayerCollisionWithFemaleEvent" : "CBPCPlayerCollisionWithMaleEvent");
								SKSEModCallbackEvent modEvent(modEventName, t.first, Globals::ActorNodePlayerCollisionEventMap[actorNodeString].totalDurationFilled, a_owner);
								g_modEventDispatcher->SendEvent(&modEvent);
								LOG("Sending cbpc event for node:{} actor:{}", t.first, a_owner->formID);
							}*/
							Globals::ActorNodePlayerCollisionEventMap[actorNodeString].durationFilled = 0;
						}
					}
					else
					{
						Globals::ActorNodePlayerCollisionEventMap[actorNodeString].durationFilled = 0;
						Globals::ActorNodePlayerCollisionEventMap[actorNodeString].totalDurationFilled = 0;
					}

					if (Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].collisionInThisCycle == true) //Node got player Genital collision in this cycle
					{
						Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].durationFilled += Globals::IntervalTimeTick;
						Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].totalDurationFilled += Globals::IntervalTimeTick;

						if (Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].durationFilled >= Globals::MinimumCollisionDurationForEvent)
						{
							//TODO DISABLED FOR NOW
							/*if (g_modEventDispatcher != NULL) {
								RE::BSFixedString modEventName(a_IsFemale ? "CBPCPlayerGenitalCollisionWithFemaleEvent" : "CBPCPlayerGenitalCollisionWithMaleEvent");
								SKSEModCallbackEvent modEvent(modEventName, t.first, Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].totalDurationFilled, a_owner);
								g_modEventDispatcher->SendEvent(&modEvent);
								LOG("Sending cbpc event for node:{} actor:{}", t.first, actor->formID);
							}*/
							Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].durationFilled = 0;
						}
					}
					else {
						Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].durationFilled = 0;
						Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].totalDurationFilled = 0;
					}
				}
			}
		});
	}

	void SimObj::UpdateCollisions(RE::Actor* a_owner) {
		tbb::parallel_for_each(m_Things.begin(), m_Things.end(), [&](auto& t) {
			for (auto& tt : t.second) {
				if (tt.second.IsBellyBone){
					tt.second.thingCollisionSpheres = tt.second.CreateThingCollisionSpheres(a_owner, Globals::spine1String.c_str());
					tt.second.thingCollisionCapsules = tt.second.CreateThingCollisionCapsules(a_owner, Globals::spine1String.c_str());
				}
				else {
					tt.second.thingCollisionSpheres = tt.second.CreateThingCollisionSpheres(a_owner, tt.second.boneName.c_str());
					tt.second.thingCollisionCapsules = tt.second.CreateThingCollisionCapsules(a_owner, tt.second.boneName.c_str());
				}
			}
		});
	}



}

#include "CBPC/Globals.hpp"

#include "Collision/CollisionHub.hpp"

#include "Config/Config.hpp"


namespace {
	/*
	uint32_t kSlotMask30 = 0x00000001;
	uint32_t kSlotMask31 = 0x00000002;
	*/
	uint32_t kSlotMask32 = 0x00000004;
	/*
	uint32_t kSlotMask33 = 0x00000008;
	uint32_t kSlotMask34 = 0x00000010;
	uint32_t kSlotMask35 = 0x00000020;
	uint32_t kSlotMask36 = 0x00000040;
	uint32_t kSlotMask37 = 0x00000080;
	uint32_t kSlotMask38 = 0x00000100;
	uint32_t kSlotMask39 = 0x00000200;
	uint32_t kSlotMask40 = 0x00000400;
	uint32_t kSlotMask41 = 0x00000800;
	uint32_t kSlotMask42 = 0x00001000;
	uint32_t kSlotMask43 = 0x00002000;
	uint32_t kSlotMask44 = 0x00004000;
	uint32_t kSlotMask45 = 0x00008000;
	uint32_t kSlotMask46 = 0x00010000;
	uint32_t kSlotMask47 = 0x00020000;
	uint32_t kSlotMask48 = 0x00040000;
	uint32_t kSlotMask49 = 0x00080000;
	uint32_t kSlotMask50 = 0x00100000;
	uint32_t kSlotMask51 = 0x00200000;
	uint32_t kSlotMask52 = 0x00400000;
	uint32_t kSlotMask53 = 0x00800000;
	uint32_t kSlotMask54 = 0x01000000;
	uint32_t kSlotMask55 = 0x02000000;
	uint32_t kSlotMask56 = 0x04000000;
	uint32_t kSlotMask57 = 0x08000000;
	uint32_t kSlotMask58 = 0x10000000;
	uint32_t kSlotMask59 = 0x20000000;
	uint32_t kSlotMask60 = 0x40000000;
	uint32_t kSlotMask61 = 0x80000000;
	*/
}

namespace CBP {
	
	/*
	EventResult TESEquipEventHandler::ReceiveEvent(TESEquipEvent* evn, EventDispatcher<TESEquipEvent>* dispatcher) {
		if (!evn)
			return EventResult::kEvent_Continue;

		if (!(PlayerCharacter::GetSingleton()) || !(PlayerCharacter::GetSingleton())->loadedState)
			return EventResult::kEvent_Continue;

		if (evn->actor == nullptr)
			return EventResult::kEvent_Continue;

		if (!(evn->baseObject > 0))
			return EventResult::kEvent_Continue;

		TESForm* form = LookupFormByID(evn->baseObject);
		if (form == nullptr)
			return EventResult::kEvent_Continue;

		TESObjectARMO* armor = DYNAMIC_CAST(form, TESForm, TESObjectARMO);
		if (armor == nullptr)
			return EventResult::kEvent_Continue;

		if (isWantSlot(armor, kSlotMask32)) // body
		{
			Actor* actor = DYNAMIC_CAST(evn->actor, TESObjectREFR, Actor);

			if (actor == nullptr && actor->loadedState == nullptr)
				return EventResult::kEvent_Continue;

			auto objIt = actors.find(actor->formID);

			if (objIt == actors.end())
				return EventResult::kEvent_Continue;

			SimObj& obj = objIt->second;

			if (!obj.isBound())
				return EventResult::kEvent_Continue;

			for (auto& t : obj.things)
			{
				for (auto& tt : t.second)
				{
					if (tt.second.IsBreastBone)
					{
						tt.second.skipArmorCheck = 0;
					}
				}
			}
		}

		return EventResult::kEvent_Continue;
	}
	*/

	bool compareActorEntries(const ActorEntry& entry1, const ActorEntry& entry2) {
		return entry1.actorDistSqr < entry2.actorDistSqr;
	}


	bool ActorIsInAngle(RE::Actor* actor, float originalHeading, RE::NiPoint3 cameraPosition) {
		if (actor->IsPlayerRef()) return true;

		if (Globals::actorAngle >= 360) return true;

		if (Globals::actorAngle <= 0 && !actor->IsPlayerRef()){
			return false;
		}

		RE::NiPoint3 position = actor->loadedData->data3D->world.translate;

		float heading = 0;
		float attitude = 0;
		GetAttitudeAndHeadingFromTwoPoints(cameraPosition, RE::NiPoint3(position.x, position.y, cameraPosition.z), attitude, heading);
		heading = heading * 57.295776f;

		return AngleDifference(originalHeading, heading) <= (Globals::actorAngle * 0.5f);
	}

	void updateActors() {

		if (Globals::debugtimelog || Globals::logging) {
			if (Globals::firsttimeloginit)
			{
				Globals::firsttimeloginit = false;
				Globals::totaltime.QuadPart = 0;

			}

			QueryPerformanceFrequency(&Globals::frequency);
			QueryPerformanceCounter(&Globals::startingTime);
		}

		// We scan the cell and build the list every time - only look up things by ID once
		// we retain all state by actor ID, in a map - it's cleared on cell change

		Globals::actorEntries.clear();

		if (RE::UI* ui = RE::UI::GetSingleton()) {
			if (ui->GameIsPaused() || ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME)) return;
		}

		if (Globals::tuningModeCollision != 0 || Globals::consoleConfigReload.load()) {
			if (Globals::consoleConfigReload.load()) {
				Globals::consoleConfigReload.store(false);

				loadMasterConfig();
				loadConfig();
				loadBounceInterpolationConfig();
				loadCollisionConfig();
				loadExtraCollisionConfig();
				loadCollisionInterpolationConfig();
				LoadPlayerCollisionEventConfig();

				Globals::actors.clear();
			}
			else
			{
				Globals::frameCount++;
				if (Globals::frameCount % (120 * Globals::tuningModeCollision) == 0)
				{
					loadMasterConfig();
					loadCollisionConfig();
					loadExtraCollisionConfig();
					loadCollisionInterpolationConfig();
					LoadPlayerCollisionEventConfig();

					Globals::actors.clear();
				}
				if (Globals::frameCount >= 1000000)
					Globals::frameCount = 0;
			}
		}

		if (Globals::modPaused.load())
			return;

		if (!RE::PlayerCharacter::GetSingleton()->Is3DLoaded()) return;

		


		RE::ProcessLists* processMan = RE::ProcessLists::GetSingleton();
		RE::TESObjectCELL* cell = RE::PlayerCharacter::GetSingleton()->GetParentCell();
		if (!cell) return;

		bool cellChanged = false;

		if (cell != Globals::curCell.load())
		{
			cellChanged = (cell != nullptr && cell->GetRuntimeData().worldSpace == nullptr && std::ranges::find(Globals::notExteriorWorlds, cell->formID) == Globals::notExteriorWorlds.end()) != (Globals::curCellSpaceExterior.load() && std::ranges::find(Globals::notExteriorWorlds, Globals::curCellWorldspaceFormId.load()) == Globals::notExteriorWorlds.end());
			Globals::curCell.store(cell);
			Globals::curCellSpaceExterior.store((cell != nullptr && cell->GetRuntimeData().worldSpace == nullptr));
			Globals::curCellWorldspaceFormId.store(cell != nullptr ? cell->formID : 0);
			LOG("Cell changed. {}",(cell != nullptr && cell->GetRuntimeData().worldSpace == nullptr) ? "Exterior" : "Interior");
		}

		Globals::callCount = 0;

		bool creatureFormChange = false;
		bool playerWeightChange = false;
		//We don't wanna do this every frame; 
		if (Globals::skipFramesScanCount > 0){
			Globals::skipFramesScanCount--;
		}
		else {
			Globals::skipFramesScanCount = 180;

			if (RE::PlayerCharacter::GetSingleton()->GetRace() != nullptr) {

				float playerWeight = (RE::PlayerCharacter::GetSingleton()->GetWeight());

				if (fabsf(playerWeight - Globals::lastPlayerWeight) > 0.001f){
					playerWeightChange = true;
				}
				Globals::lastPlayerWeight = playerWeight;
				
				//In beast form
				if (RE::PlayerCharacter::GetSingleton()->GetRace()->formID == 0xCDD84 || RE::PlayerCharacter::GetSingleton()->GetRace()->formID == Globals::VampireLordBeastRaceFormId) {
					if (Globals::inCreatureForm == false) {
						creatureFormChange = true;
						Globals::inCreatureForm = true;
					}
				}
				else {
					if (Globals::inCreatureForm == true) {
						creatureFormChange = true;
						Globals::inCreatureForm = false;
					}
				}
			}
		}

		if (cellChanged || Globals::raceSexMenuClosed.load() || creatureFormChange || playerWeightChange || Globals::MainMenuOpen.load()) {
			Globals::raceSexMenuClosed.store(false);
			Globals::MainMenuOpen.store(false);
			Globals::actors.clear();
		}
		else {
			if (processMan) {
				float originalHeading = 0;
				RE::NiPoint3 cameraPosition;
				RE::NiPoint3 cameraHeadingPosition;

				if (Globals::useCamera)
				{
					RE::PlayerCamera* camera = RE::PlayerCamera::GetSingleton();

					if (camera != nullptr && camera->cameraRoot != nullptr)
					{
						cameraPosition = camera->cameraRoot->world.translate + (camera->cameraRoot->world.rotate * RE::NiPoint3(0, -200.0f, 0));
						cameraHeadingPosition = cameraPosition + (camera->cameraRoot->world.rotate * RE::NiPoint3(0, 500.0f, 0));
					}
					else
					{
						cameraPosition = RE::PlayerCharacter::GetSingleton()->loadedData->data3D->world.translate + ((RE::PlayerCharacter::GetSingleton()->loadedData->data3D->world.rotate * RE::NiPoint3(0, -200.0f, 0)));
						cameraHeadingPosition = cameraPosition + (RE::PlayerCharacter::GetSingleton()->loadedData->data3D->world.rotate * RE::NiPoint3(0, 500.0f, 0));
					}
				}
				else
				{
					cameraPosition = RE::PlayerCharacter::GetSingleton()->loadedData->data3D->world.translate + (RE::PlayerCharacter::GetSingleton()->loadedData->data3D->world.rotate * RE::NiPoint3(0, -200.0f, 0));
					cameraHeadingPosition = cameraPosition + (RE::PlayerCharacter::GetSingleton()->loadedData->data3D->world.rotate * RE::NiPoint3(0, 500.0f, 0));
				}

				float heading = 0;
				float attitude = 0;
				GetAttitudeAndHeadingFromTwoPoints(cameraPosition, cameraHeadingPosition, attitude, heading);
				originalHeading = heading * 57.295776f;

				RE::NiPoint3 relativeActorPos;

				tbb::parallel_for(0, processMan->numberHighActors + 1, [&](uint32_t i) {
	
					RE::Actor* actor;
					RE::NiPointer<RE::TESObjectREFR> ToRefr = nullptr;
					bool isValid = true;

					if (i < processMan->numberHighActors) {
						ToRefr = processMan->highActorHandles[i].get();
					}

					if (ToRefr != nullptr || i == processMan->numberHighActors) {
						if (i == processMan->numberHighActors) {
							actor = RE::PlayerCharacter::GetSingleton();
						}
						else {
							actor = skyrim_cast<RE::Actor*>(ToRefr.get());
						}

						if (actor && actor->loadedData && actor->loadedData->data3D)
						{
							if (actor->GetRace() && !actor->IsPlayerRef())
							{
								std::string actorRace = actor->GetRace()->fullName.c_str();

								if (actor->GetRace()->IsChildRace())
								isValid = false;

								LOG("actorRace: {}", actorRace.c_str());
							}

							if (actor->loadedData->data3D.get() && isValid)
							{
								relativeActorPos = actor->loadedData->data3D->world.translate - RE::PlayerCharacter::GetSingleton()->loadedData->data3D->world.translate;

								float actorDistSqr = magnitudePwr2(relativeActorPos);

								if (!actor->IsPlayerTeammate())
								{
									if (actorDistSqr > Globals::actorBounceDistance)
										isValid = false;

									if (!ActorIsInAngle(actor, originalHeading, cameraPosition)){
										isValid = false;
									}
								}

								if (Globals::actors.find(actor->formID) == Globals::actors.end() && isValid) //If actor isn't in the actors list
								{
									//logger.info("Tracking Actor with form ID {} in cell {}\n", actor->formID, actor->parentCell);
									if (IsActorValid(actor))
									{
										auto obj = SimObj(actor);
										obj.m_ActorDistanceSqr = actorDistSqr;
										Globals::actors.insert(std::make_pair(actor->formID, obj));
										Globals::actorEntries.push_back(ActorEntry{ actor->formID, actor, IsActorMale(actor) , actorDistSqr, actorDistSqr <= Globals::actorDistance });
									}
								}
								else if (IsActorValid(actor) && isValid)
								{
									Globals::actors[actor->formID].m_ActorDistanceSqr = actorDistSqr;
									Globals::actorEntries.push_back(ActorEntry{ actor->formID, actor, IsActorMale(actor), actorDistSqr, actorDistSqr <= Globals::actorDistance });
								}
							}
						}
					}
				});
			}
		}


		if (RE::PlayerCharacter::GetSingleton()->IsInCombat())
		{
			if (Globals::actorEntries.size() > Globals::inCombatActorCount)
			{
				std::ranges::sort(Globals::actorEntries, compareActorEntries);

				Globals::actorEntries.resize(Globals::inCombatActorCount);
			}
		}
		else
		{
			if (Globals::actorEntries.size() > Globals::outOfCombatActorCount)
			{
				std::ranges::sort(Globals::actorEntries, compareActorEntries);

				Globals::actorEntries.resize(Globals::outOfCombatActorCount);
			}
		}

		LOG("ActorCount: {}", Globals::actorEntries.size());

		Globals::partitions.clear();

		LOG("Starting collider hashing");


		RE::NiPoint3 playerPos = RE::PlayerCharacter::GetSingleton()->GetPosition();
		long colliderSphereCount = 0;
		long colliderCapsuleCount = 0;

		tbb::parallel_for(static_cast<size_t>(0), Globals::actorEntries.size(), [&](size_t u) {
				if (Globals::actorEntries[u].collisionsEnabled == true)
				{
					auto objIt = Globals::actors.find(Globals::actorEntries[u].id);
					if (objIt != Globals::actors.end())

					{
						UpdateColliderPositions(objIt->second.m_ActorColliders, objIt->second.m_NodeCollisionSync);

						tbb::parallel_for_each(objIt->second.m_ActorColliders.begin(), objIt->second.m_ActorColliders.end(), [&](const auto& collider)
							{
								std::vector<int> ids;
								std::vector<int> hashIdList;
								for (int j = 0; j < collider.second.collisionSpheres.size(); j++)
								{
									hashIdList = GetHashIdsFromPos(collider.second.collisionSpheres[j].worldPos - playerPos, collider.second.collisionSpheres[j].radius100);

									for (int m = 0; m < hashIdList.size(); m++)
									{
										if (std::ranges::find(ids, hashIdList[m]) == ids.end())
										{
											LOG_INFO("ids.emplace_back({})", hashIdList[m]);
											ids.emplace_back(hashIdList[m]);
											Globals::partitions[hashIdList[m]].partitionCollisions.push_back(collider.second);
										}
									}
									if (Globals::logging)
										InterlockedIncrement(&colliderSphereCount);
								}

								for (int j = 0; j < collider.second.collisionCapsules.size(); j++)
								{
									hashIdList = GetHashIdsFromPos((collider.second.collisionCapsules[j].End1_worldPos + collider.second.collisionCapsules[j].End2_worldPos) * 0.5f - playerPos
										, (collider.second.collisionCapsules[j].End1_radius100 + collider.second.collisionCapsules[j].End2_radius100) * 0.5f);
									for (int m = 0; m < hashIdList.size(); m++)
									{
										if (std::ranges::find(ids, hashIdList[m]) == ids.end())
										{
											LOG_INFO("ids.emplace_back({})", hashIdList[m]);
											ids.emplace_back(hashIdList[m]);
											Globals::partitions[hashIdList[m]].partitionCollisions.push_back(collider.second);
										}
									}
									if (Globals::logging)
										InterlockedIncrement(&colliderCapsuleCount);
								}
						});
					}
				}
			});
		LOG("Collider sphere count = {}", colliderSphereCount);
		LOG("Collider capsule count = {}", colliderCapsuleCount);

		static int count = 0;
		if ((Globals::configReloadCount && count++ > Globals::configReloadCount))
		{
			count = 0;
			loadConfig();
			loadBounceInterpolationConfig();
			for (auto& a : Globals::actorEntries)
			{
				auto objIt = Globals::actors.find(a.id);
				if (objIt == Globals::actors.end())
				{
					//logger.error("missing Sim Object\n");
				}
				else
				{
					if (a.actor != nullptr && a.actor->loadedData != nullptr)
					{
						SimObj& obj = objIt->second;
						obj.UpdateConfig(a.actor);
					}
				}
			}
		}
		else
		{
			Globals::actorBounceRefreshMap_lock.lock();
			if (Globals::actorFormIdRefreshBounceSettingsMap.size() > 0)
			{
				LOG_ERR("Refreshing actor Bounce...");
				tbb::parallel_for_each(Globals::actorFormIdRefreshBounceSettingsMap.begin(), Globals::actorFormIdRefreshBounceSettingsMap.end(), [&](auto& innerPair)
				{
					if (innerPair.second)
					{
						innerPair.second = false;

						auto objIt = Globals::actors.find(innerPair.first);
						if (objIt != Globals::actors.end())
						{
							SimObj& obj = objIt->second;
							if (obj.m_OwnerActor != nullptr && obj.m_OwnerActor->loadedData != nullptr)
							{
								LOG_ERR("Refreshing actor bounce for {}", obj.m_OwnerActor->formID);
								obj.UpdateConfig(obj.m_OwnerActor);
							}
						}
					}
				});
				Globals::actorFormIdRefreshBounceSettingsMap.clear();
			}
			Globals::actorBounceRefreshMap_lock.unlock();
		}

		Globals::actorCollisionRefreshMap_lock.lock();

		if (Globals::actorFormIdRefreshCollisionSettingsMap.size() > 0)
		{
			LOG_ERR("Refreshing actor collisions...");

			tbb::parallel_for_each(Globals::actorFormIdRefreshCollisionSettingsMap.begin(), Globals::actorFormIdRefreshCollisionSettingsMap.end(), [&](auto& innerPair)
			{
				if (innerPair.second)
				{
					innerPair.second = false;

					auto objIt = Globals::actors.find(innerPair.first);
					if (objIt != Globals::actors.end())
					{
						SimObj& obj = objIt->second;
						if (obj.m_OwnerActor != nullptr && obj.m_OwnerActor->loadedData != nullptr)
						{
							LOG_ERR("Refreshing actor collisions for {}", obj.m_OwnerActor->formID);
							obj.UpdateCollisions(obj.m_OwnerActor);
						}
					}
				}
			});
			Globals::actorFormIdRefreshCollisionSettingsMap.clear();
		}
		Globals::actorCollisionRefreshMap_lock.unlock();
		//logger.error("Updating {} entites\n", actorEntries.size());

		//Get DeltaT by engine
		Globals::IntervalTimeTick = GetFrameIntervalTimeTick();
		Globals::IntervalTimeTickScale = Globals::IntervalTimeTick / Globals::IntervalTime60Tick;
		tbb::parallel_for_each(Globals::actorEntries.begin(), Globals::actorEntries.end(), [&](const auto& a)
		{
			auto objIt = Globals::actors.find(a.id);
			if (objIt == Globals::actors.end())
			{
				//logger.error("missing Sim Object\n");
			}
			else
			{
				if (a.actor != nullptr && a.actor->loadedData != nullptr)
				{
					SimObj& obj = objIt->second;
					if (obj.IsBound())
					{
						obj.Update(a.actor, a.collisionsEnabled, a.sex == 0);
					}
					else
					{
						obj.Bind(a.actor, a.sex == 1);
					}
				}
			}
		});

		LOG("Collider Check Call Count: {}", Globals::callCount);

		if (Globals::debugtimelog || Globals::logging)
		{
			QueryPerformanceCounter(&Globals::endingTime);
			Globals::elapsedMicroseconds.QuadPart = Globals::endingTime.QuadPart - Globals::startingTime.QuadPart;
			Globals::elapsedMicroseconds.QuadPart *= 1000000000LL;
			Globals::elapsedMicroseconds.QuadPart /= Globals::frequency.QuadPart;
			//long long avg = elapsedMicroseconds.QuadPart / callCount;
			Globals::totaltime.QuadPart += Globals::elapsedMicroseconds.QuadPart;
			Globals::totalcallcount += Globals::callCount;
			LOG_ERR("Collider Check Call Count: {} - Update Time = {} ns", Globals::callCount, Globals::elapsedMicroseconds.QuadPart);
			if (Globals::debugtimelog_framecount % 1000 == 0)
			{
				LOG_ERR("Collider Check Call Count: %.2f - Average Update Time in 1000 frame = {} ns\n", static_cast<float>(Globals::totalcallcount) / static_cast<float>(Globals::debugtimelog_framecount), Globals::totaltime.QuadPart / Globals::debugtimelog_framecount);
				Globals::totaltime.QuadPart = 0;
				Globals::debugtimelog_framecount = 0;
				Globals::totalcallcount = 0;
			}
			Globals::debugtimelog_framecount++;
		}
		//logger.info("Update Time = {} ns\n", elapsedMicroseconds.QuadPart);

		return;

	}

}

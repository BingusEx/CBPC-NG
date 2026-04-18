/*
namespace CBP {
	

	bool RegisterFuncs(VMClassRegistry* registry)
	{

		registry->RegisterFunction(
			new NativeFunction0<StaticFunctionTag, BSFixedString>("GetVersion", "CBPCPluginScript", GetVersion, registry));

		registry->RegisterFunction(
			new NativeFunction0<StaticFunctionTag, BSFixedString>("GetVersionMinor", "CBPCPluginScript", GetVersionMinor, registry));

		registry->RegisterFunction(
			new NativeFunction0<StaticFunctionTag, BSFixedString>("GetVersionBeta", "CBPCPluginScript", GetVersionBeta, registry));

		registry->RegisterFunction(
			new NativeFunction0 <StaticFunctionTag, void>("ReloadConfig", "CBPCPluginScript", ReloadConfig, registry));

		registry->RegisterFunction(
			new NativeFunction2 <StaticFunctionTag, void, Actor*, BSFixedString>("StartPhysics", "CBPCPluginScript", StartPhysics, registry));

		registry->RegisterFunction(
			new NativeFunction2 <StaticFunctionTag, void, Actor*, BSFixedString>("StopPhysics", "CBPCPluginScript", StopPhysics, registry));

		registry->RegisterFunction(
			new NativeFunction7 <StaticFunctionTag, bool, Actor*, BSFixedString, VMArray<float>, float, float, uint32_t, bool>("AttachColliderSphere", "CBPCPluginScript", AttachColliderSphere, registry));

		registry->RegisterFunction(
			new NativeFunction9 <StaticFunctionTag, bool, Actor*, BSFixedString, VMArray<float>, float, VMArray<float>, float, float, uint32_t, bool>("AttachColliderCapsule", "CBPCPluginScript", AttachColliderCapsule, registry));
		registry->RegisterFunction(
			new NativeFunction5 <StaticFunctionTag, bool, Actor*, BSFixedString, uint32_t, uint32_t, bool>("DetachCollider", "CBPCPluginScript", DetachCollider, registry));

		registry->RegisterFunction(
			new NativeFunction3 <StaticFunctionTag, void, Actor*, BSFixedString, uint32_t>("ApplyCollisionInterpolation", "CBPCPluginScript", ApplyCollisionInterpolation, registry));

		registry->RegisterFunction(
			new NativeFunction3 <StaticFunctionTag, void, Actor*, BSFixedString, uint32_t>("ApplyBounceInterpolation", "CBPCPluginScript", ApplyBounceInterpolation, registry));

		registry->RegisterFunction(
			new NativeFunction1 <StaticFunctionTag, void, Actor*>("RefreshActorBounceSettings", "CBPCPluginScript", RefreshActorBounceSettings, registry));

		registry->RegisterFunction(
			new NativeFunction1 <StaticFunctionTag, void, Actor*>("RefreshActorCollisionSettings", "CBPCPluginScript", RefreshActorCollisionSettings, registry));

		LOG("CBPC registerFunction\n");
		return true;
	}

	void StartPhysics(StaticFunctionTag* base, Actor* actor, BSFixedString nodeName)
	{
		if (actor != nullptr)
		{
			ActorNodeStoppedPhysicsMap[GetActorNodeString(actor, nodeName)] = false;
		}
	}

	void StopPhysics(StaticFunctionTag* base, Actor* actor, BSFixedString nodeName)
	{
		if (actor != nullptr)
		{
			ActorNodeStoppedPhysicsMap[GetActorNodeString(actor, nodeName)] = true;
		}
	}

	void ApplyBounceInterpolation(StaticFunctionTag* base, Actor* actor, BSFixedString uniqueName, uint32_t interpolatePercentage)
	{
		if (actor != nullptr)
		{
			actorBounceInterpolationList_lock.lock();
			std::string uniqueNameStr = uniqueName.c_str();
			transform(uniqueNameStr.begin(), uniqueNameStr.end(), uniqueNameStr.begin(), ::tolower);
			actorBounceInterpolationsList[actor->formID][uniqueNameStr.c_str()] = interpolatePercentage;
			actorBounceInterpolationList_lock.unlock();
			LOG("Applied bounce interpolation for {} - {} - {}", actor->formID, uniqueNameStr.c_str(), interpolatePercentage);
		}
	}

	void ApplyCollisionInterpolation(StaticFunctionTag* base, Actor* actor, BSFixedString uniqueName, uint32_t interpolatePercentage)
	{
		if (actor != nullptr)
		{
			actorCollisionInterpolationList_lock.lock();
			std::string uniqueNameStr = uniqueName.c_str();
			transform(uniqueNameStr.begin(), uniqueNameStr.end(), uniqueNameStr.begin(), ::tolower);
			actorCollisionInterpolationsList[actor->formID][uniqueNameStr.c_str()] = interpolatePercentage;
			actorCollisionInterpolationList_lock.unlock();
			LOG("Applied Collision interpolation for {} - {} - {}", actor->formID, uniqueNameStr.c_str(), interpolatePercentage);
		}
	}


	void RefreshActorBounceSettings(StaticFunctionTag* base, Actor* actor)
	{
		if (actor != nullptr)
		{
			actorBounceRefreshMap_lock.lock();
			actorFormIdRefreshBounceSettingsMap[actor->formID] = true;
			actorBounceRefreshMap_lock.unlock();
			LOG("Refresh actor bounce for {}", actor->formID);
		}
	}

	void RefreshActorCollisionSettings(StaticFunctionTag* base, Actor* actor)
	{
		if (actor != nullptr)
		{
			actorCollisionRefreshMap_lock.lock();
			actorFormIdRefreshCollisionSettingsMap[actor->formID] = true;
			actorCollisionRefreshMap_lock.unlock();
			LOG("Refresh actor collision for {}", actor->formID);
		}
	}

	BSFixedString GetVersion(StaticFunctionTag* base)
	{
		return BSFixedString("1");
	}

	BSFixedString GetVersionMinor(StaticFunctionTag* base)
	{
		return BSFixedString("6");
	}

	BSFixedString GetVersionBeta(StaticFunctionTag* base)
	{
		return BSFixedString("2");
	}

	void ReloadConfig(StaticFunctionTag* base)
	{
		consoleConfigReload.store(true);
	}

	//papyrus scripts for collider attach or detach
	bool AttachColliderSphere(StaticFunctionTag* base, Actor* actor, BSFixedString nodeName, VMArray<float> position, float radius, float scaleWeight, uint32_t index, bool IsAffectedNodes)
	{
		if (!actor || !actor->loadedState || !actor->loadedData->data3D)
			return false;

		NiAVObject* node = actor->loadedData->data3D->GetObjectByName(nodeName.c_str());

		if (!node)
			return false;

		if (position.Length() != 3)
			return false;

		NiPoint3 nodePosition = {};
		position.Get(&nodePosition.x, 0);
		position.Get(&nodePosition.y, 1);
		position.Get(&nodePosition.z, 2);

		Sphere newSphere;

		newSphere.offset0 = nodePosition;
		newSphere.offset100 = newSphere.offset0;
		newSphere.radius0 = radius;
		newSphere.radius100 = newSphere.radius0;
		newSphere.index = index;
		newSphere.NodeName = nodeName.data;

		auto objIt = actors.find(actor->formID);
		if (objIt == actors.end())
			return false;

		if (!IsAffectedNodes)
		{
			auto& colliders = objIt->second.actorColliders;

			if (colliders.find(nodeName.data) != colliders.end())
			{
				auto& col = colliders.find(nodeName.data)->second;

				col.collisionSpheres.emplace_back(newSphere);
			}
			else
			{
				std::vector<Sphere>newSpherelist;
				std::vector<Capsule>newCapsulelist;

				newSpherelist.emplace_back(newSphere);

				Collision newCol = Collision::Collision(node, newSpherelist, newCapsulelist, 0.0f);
				newCol.colliderActor = actor;
				newCol.colliderNodeName = nodeName.data;

				if (scaleWeight > 1.0f)
					newCol.scaleWeight = 1.0f;
				else if (scaleWeight < 0.0f)
					newCol.scaleWeight = 0.0f;
				else
					newCol.scaleWeight = scaleWeight;

				auto actorRef = DYNAMIC_CAST(actor, Actor, TESObjectREFR);
				float actorBaseScale = 1.0f;
				if (actorRef)
					actorBaseScale = CALL_MEMBER_FN(actorRef, GetBaseScale)();

				newCol.actorBaseScale = actorBaseScale;

				colliders.insert(std::make_pair(newCol.colliderNodeName, newCol));
			}

			return true;
		}
		else
		{
			auto& things = objIt->second.things;

			bool isthereThing = false;

			for (auto& t : things)
			{
				for (auto& tt : t.second)
				{
					if (strcmp(tt.first, nodeName.data) == 0)
					{
						isthereThing = true;

						auto& ttIt = tt.second;

						ttIt.thingCollisionSpheres.emplace_back(newSphere);
					}
				}
			}

			return isthereThing;
		}
		return false;
	}

	bool AttachColliderCapsule(StaticFunctionTag* base, Actor* actor, BSFixedString nodeName, VMArray<float> End1_position, float End1_radius, VMArray<float> End2_position, float End2_radius, float scaleWeight, uint32_t index, bool IsAffectedNodes)
	{
		if (!actor || !actor->loadedState || !actor->loadedData->data3D)
			return false;

		NiAVObject* node = actor->loadedData->data3D->GetObjectByName(&nodeName.data);

		if (!node)
			return false;

		if (End1_position.Length() != 3 || End2_position.Length() != 3)
			return false;

		NiPoint3 End1_nodePosition = {};
		NiPoint3 End2_nodePosition = {};

		End1_position.Get(&End1_nodePosition.x, 0);
		End1_position.Get(&End1_nodePosition.y, 1);
		End1_position.Get(&End1_nodePosition.z, 2);

		End2_position.Get(&End2_nodePosition.x, 0);
		End2_position.Get(&End2_nodePosition.y, 1);
		End2_position.Get(&End2_nodePosition.z, 2);

		Capsule newCapsule;

		newCapsule.End1_offset0 = End1_nodePosition;
		newCapsule.End1_offset100 = newCapsule.End1_offset0;
		newCapsule.End1_radius0 = End1_radius;
		newCapsule.End1_radius100 = newCapsule.End1_radius0;
		newCapsule.End2_offset0 = End2_nodePosition;
		newCapsule.End2_offset100 = newCapsule.End2_offset0;
		newCapsule.End2_radius0 = End2_radius;
		newCapsule.End2_radius100 = newCapsule.End2_radius0;
		newCapsule.index = index;
		newCapsule.NodeName = nodeName.data;

		auto objIt = actors.find(actor->formID);
		if (objIt == actors.end())
			return false;

		if (!IsAffectedNodes)
		{
			auto& colliders = objIt->second.actorColliders;

			if (colliders.find(nodeName.data) != colliders.end())
			{
				auto& col = colliders.find(nodeName.data)->second;

				col.collisionCapsules.emplace_back(newCapsule);
			}
			else
			{
				std::vector<Sphere>newSpherelist;
				std::vector<Capsule>newCapsulelist;

				newCapsulelist.emplace_back(newCapsule);

				Collision newCol = Collision::Collision(node, newSpherelist, newCapsulelist, 50.0f);
				newCol.colliderActor = actor;
				newCol.colliderNodeName = nodeName.data;

				if (scaleWeight > 1.0f)
					newCol.scaleWeight = 1.0f;
				else if (scaleWeight < 0.0f)
					newCol.scaleWeight = 0.0f;
				else
					newCol.scaleWeight = scaleWeight;

				auto actorRef = DYNAMIC_CAST(actor, Actor, TESObjectREFR);
				float actorBaseScale = 1.0f;
				if (actorRef)
					actorBaseScale = CALL_MEMBER_FN(actorRef, GetBaseScale)();

				newCol.actorBaseScale = actorBaseScale;

				colliders.insert(std::make_pair(newCol.colliderNodeName, newCol));
			}
			return true;
		}
		else
		{
			auto& things = objIt->second.things;

			bool isthereThing = false;

			for (auto& t : things)
			{
				for (auto& tt : t.second)
				{
					if (strcmp(tt.first, nodeName.data) == 0)
					{
						isthereThing = true;

						auto& ttIt = tt.second;

						ttIt.thingCollisionCapsules.emplace_back(newCapsule);
					}
				}
			}

			return isthereThing;
		}
		return false;
	}

	bool DetachCollider(StaticFunctionTag* base, Actor* actor, BSFixedString nodeName, uint32_t type, uint32_t index, bool IsAffectedNodes) // type 0 = sphere / type 1 = capsule
	{
		if (!actor || !actor->loadedState || !actor->loadedData->data3D)
			return false;

		NiAVObject* node = actor->loadedData->data3D->GetObjectByName(&nodeName.data);

		if (!node)
			return false;

		auto objIt = actors.find(actor->formID);
		if (objIt == actors.end())
			return false;

		if (!IsAffectedNodes)
		{
			auto& colliders = objIt->second.actorColliders;

			if (colliders.find(nodeName.data) == colliders.end())
				return false;

			auto& col = colliders.find(nodeName.data)->second;

			bool isremoveThere = false;
			if (type == 0)
			{
				int i = 0;
				while (i < col.collisionSpheres.size())
				{
					if (col.collisionSpheres.at(i).index == index)
					{
						isremoveThere = true;
						col.collisionSpheres.erase(col.collisionSpheres.begin() + i);
					}
					else
						i++;
				}
			}
			else if (type == 1)
			{
				int i = 0;
				while (i < col.collisionCapsules.size())
				{
					if (col.collisionCapsules.at(i).index == index)
					{
						isremoveThere = true;
						col.collisionCapsules.erase(col.collisionCapsules.begin() + i);
					}
					else
						i++;
				}
			}

			return isremoveThere;
		}
		else
		{
			auto& things = objIt->second.things;

			bool isremoveThere = false;

			for (auto& t : things)
			{
				for (auto& tt : t.second)
				{
					if (strcmp(tt.first, nodeName.data) == 0)
					{
						auto& ttIt = tt.second;

						if (type == 0)
						{
							int i = 0;
							while (i < ttIt.thingCollisionSpheres.size())
							{
								if (ttIt.thingCollisionSpheres.at(i).index == index)
								{
									isremoveThere = true;
									ttIt.thingCollisionSpheres.erase(ttIt.thingCollisionSpheres.begin() + i);
								}
								else
									i++;
							}
						}
						else if (type == 1)
						{
							int i = 0;
							while (i < ttIt.thingCollisionCapsules.size())
							{
								if (ttIt.thingCollisionCapsules.at(i).index == index)
								{
									isremoveThere = true;
									ttIt.thingCollisionCapsules.erase(ttIt.thingCollisionCapsules.begin() + i);
								}
								else
									i++;
							}
						}
					}
				}
			}

			return isremoveThere;
		}
		return false;
	}







}
*/
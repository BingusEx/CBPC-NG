#include "CBPC/Thing.hpp"

#include "Globals.hpp"

#include "Collision/CollisionHub.hpp"

#include "Config/Config.hpp"

namespace {
	std::shared_mutex thing_map_lock;
}

namespace CBP {

	Thing::Thing(RE::Actor* actor, RE::NiAVObject* obj, RE::BSFixedString& name) : velocity(RE::NiPoint3(0, 0, 0)) , velocityRot(RE::NiPoint3(0, 0, 0)) , boneName(name)
	{
		if (actor)
		{
			if (actor->loadedData && actor->loadedData->data3D)
			{
				if (obj)
				{
					ownerActor = actor;
					node = obj;
				}
				else
				{
					return;
				}
			}
			else
			{
				return;
			}
		}
		else
		{
			return;
		}

		oldWorldPos = node->parent->world.translate;
		oldWorldPosRot = node->parent->world.translate;
		time = clock();

		IsLeftBreastBone = ContainsNoCase(boneName.c_str(), "L Breast");
		IsRightBreastBone = ContainsNoCase(boneName.c_str(), "R Breast");
		IsBreastBone = ContainsNoCase(boneName.c_str(), "Breast");
		IsBellyBone = strcmp(boneName.c_str(), Globals::belly.c_str()) == 0;

		actorNodeString = GetActorNodeString(actor, boneName);

		if (IsBellyBone)
		{
			thingCollisionSpheres = CreateThingCollisionSpheres(actor, Globals::spine1.c_str()); //suggest reading comments on the bellybullge function
			thingCollisionCapsules = CreateThingCollisionCapsules(actor, Globals::spine1.c_str());
		}
		else
		{
			thingCollisionSpheres = CreateThingCollisionSpheres(actor, name.c_str());
			thingCollisionCapsules = CreateThingCollisionCapsules(actor, name.c_str());
		}

		if (updateThingFirstRun)
		{
			updateThingFirstRun = false;
			oldBellyBulge = newBellyBulge;
			updateBellyBulge = true;

			std::pair<unsigned, const char*> mypair = std::make_pair(actor->GetActorBase()->formID, name.c_str());

			thing_map_lock.lock();
			std::map<std::pair<uint32_t, std::string_view>, RE::NiPoint3>::iterator posMap =Globals::thingDefaultPosList.find(mypair);

			if (posMap == Globals::thingDefaultPosList.end())
			{
				if (strcmp(name.c_str(), Globals::belly.c_str()) == 0)
				{
					thingDefaultPos = {};
				}
				else
				{
					//Add it to the list
					thingDefaultPos = obj->local.translate;
				}
				Globals::thingDefaultPosList[mypair] = thingDefaultPos;
				LOG("Adding {} to default list for {} -> {} {} {}", name.c_str(), actor->GetActorBase()->formID, thingDefaultPos.x, thingDefaultPos.y, thingDefaultPos.z);
			}
			else
			{
				if (strcmp(name.c_str(), Globals::belly.c_str()) == 0)
				{
					thingDefaultPos = {};
				}
				else
				{
					thingDefaultPos = posMap->second;
				}
			}
			LOG_INFO("{} default pos -> {} {} {}", boneName.c_str(), thingDefaultPos.x, thingDefaultPos.y, thingDefaultPos.z);
			std::map<std::pair<unsigned, std::string_view>, RE::NiMatrix3>::iterator rotMap = Globals::thingDefaultRotList.find(mypair);

			if (rotMap == Globals::thingDefaultRotList.end())
			{
				//Add it to the list
				thingDefaultRot = obj->local.rotate;
				Globals::thingDefaultRotList[mypair] = thingDefaultRot;
			}
			else
			{
				thingDefaultRot = rotMap->second;
			}
			thing_map_lock.unlock();
			collisionBuffer = {};
			collisionSync = {};
		}

		skipFramesCount = Globals::collisionSkipFrames;
		skipFramesPelvisCount = Globals::collisionSkipFramesPelvis;
	}

	Thing::~Thing() {}

	bool areSpheresEqual(const Sphere& sphere1, const Sphere& sphere2)
	{
		return CompareNiPoints(sphere1.offset0, sphere2.offset0) && sphere1.radius0 == sphere2.radius0;
	}

	Sphere interpolateTwoSpheres(const Sphere& sphere1, const Sphere& sphere2, float percentage)
	{
		Sphere result;

		result.offset0 = GetPointFromPercentage(sphere1.offset0, sphere2.offset0, percentage);

		result.radius0 = GetPercentageValue(sphere1.radius0, sphere2.radius100, percentage);

		result.radius100pwr2 = result.radius0 * result.radius0;

		return result;
	}

	void interpolateSpheres(std::vector<Sphere>& initialSpheres, const std::vector<Sphere>& targetSpheres, float percentage)
	{
		size_t initialCount = initialSpheres.size();
		size_t targetCount = targetSpheres.size();
		LOG_ERR("Interpolating {} spheres with {} spheres", initialCount, targetCount);
		// Interpolate spheres individually
		if (initialCount > 0 && targetCount > 0)
		{
			for (size_t i = 0; i < std::min(initialCount, targetCount); ++i)
			{
				initialSpheres[i] = interpolateTwoSpheres(initialSpheres[i], targetSpheres[i], percentage);
			}
		}

		// If initialCount < targetCount, interpolate towards the last target sphere after 0.5
		if (initialCount < targetCount && percentage >= 50 && initialCount > 0)
		{
			for (size_t i = initialCount; i < targetCount; ++i)
			{
				initialSpheres.push_back(interpolateTwoSpheres(initialSpheres[i % initialCount], targetSpheres[i], 2 * (percentage - 50)));
			}
		}
		// If initialCount > targetCount, duplicate target spheres and interpolate
		else if (initialCount > targetCount && percentage < 50 && targetCount > 0)
		{
			for (size_t i = std::min(initialCount, targetCount); i < initialCount; ++i)
			{
				initialSpheres[i] = interpolateTwoSpheres(initialSpheres[i], targetSpheres[i % targetCount], 2 * percentage);
			}
		}

		std::ranges::sort(initialSpheres, [](const Sphere& a, const Sphere& b) {
			if (a.offset0.x != b.offset0.x) return a.offset0.x < b.offset0.x;
			if (a.offset0.y != b.offset0.y) return a.offset0.y < b.offset0.y;
			if (a.offset0.z != b.offset0.z) return a.offset0.z < b.offset0.z;
			return a.radius0 < b.radius0;
		});

		// Use std::unique to remove adjacent duplicates
		auto last = std::unique(initialSpheres.begin(), initialSpheres.end());

		// Erase the duplicates from the vector
		initialSpheres.erase(last, initialSpheres.end());
	}

	std::vector<Sphere> Thing::CreateThingCollisionSpheres(RE::Actor* actor, const std::string& nodeName)
	{

		if (!actor) return {};

		actorWeight = actor->GetWeight();

		actorBaseScale = actor->GetBaseHeight();
		tbb::concurrent_vector<ConfigLine>* AffectedNodesListPtr;

		std::string actorrefname = "";
		std::string actorRace = "";

		SpecificNPCConfig snc;

		if (actor->formID == 0x14) //If Player
		{
			actorrefname = "Player";
		}
		else
		{
			actorrefname = actor->GetDisplayFullName();
		}

		if (actor->GetRace())
		{
			actorRace = actor->GetRace()->fullName.c_str();
		}

		bool success = GetSpecificNPCConfigForActor(actor, snc);

		if (success)
		{
			AffectedNodesListPtr = &(snc.AffectedNodesList);
			thing_bellybulgemultiplier = snc.cbellybulge;
			thing_bellybulgemax = snc.cbellybulgemax;
			thing_bellybulgeposlowest = snc.cbellybulgeposlowest;
			thing_bellybulgelist = snc.bellybulgenodesList;
			thing_bellybulgeblacklist = snc.bellybulgenodesBlackList;
			thing_bellyBulgeSpeed = snc.bellyBulgeSpeed;
			thing_vaginaOpeningLimit = snc.vaginaOpeningLimit;
			thing_vaginaOpeningMultiplier = snc.vaginaOpeningMultiplier;
			thing_anusOpeningLimit = snc.anusOpeningLimit;
			thing_anusOpeningMultiplier = snc.anusOpeningMultiplier;
		}
		else
		{
			AffectedNodesListPtr = &Globals::AffectedNodesList;
			thing_bellybulgemultiplier = Globals::cbellybulge;
			thing_bellybulgemax = Globals::cbellybulgemax;
			thing_bellybulgeposlowest = Globals::cbellybulgeposlowest;
			thing_bellybulgelist = Globals::bellybulgenodesList;
			thing_bellybulgeblacklist = Globals::bellybulgenodesBlackList;
			thing_bellyBulgeSpeed = Globals::bellyBulgeSpeed;
			thing_vaginaOpeningLimit = Globals::vaginaOpeningLimit;
			thing_vaginaOpeningMultiplier = Globals::vaginaOpeningMultiplier;
			thing_anusOpeningLimit = Globals::anusOpeningLimit;
			thing_anusOpeningMultiplier = Globals::anusOpeningMultiplier;
		}

		std::vector<Sphere> spheres;

		tbb::parallel_for(static_cast<size_t>(0), AffectedNodesListPtr->size(), [&](size_t i)
			{
				if (AffectedNodesListPtr->at(i).NodeName == nodeName)
				{
					spheres = AffectedNodesListPtr->at(i).CollisionSpheres;
					IgnoredCollidersList = AffectedNodesListPtr->at(i).IgnoredColliders;
					IgnoredSelfCollidersList = AffectedNodesListPtr->at(i).IgnoredSelfColliders;
					IgnoreAllSelfColliders = AffectedNodesListPtr->at(i).IgnoreAllSelfColliders;
					for (int j = 0; j < spheres.size(); j++)
					{
						spheres[j].offset0 = GetPointFromPercentage(spheres[j].offset0, spheres[j].offset100, actorWeight);

						spheres[j].radius0 = GetPercentageValue(spheres[j].radius0, spheres[j].radius100, actorWeight);

						spheres[j].radius100pwr2 = spheres[j].radius0 * spheres[j].radius0;
					}

					scaleWeight = AffectedNodesListPtr->at(i).scaleWeight;
				}
			});

		std::vector<Sphere> targetSpheres;

		Globals::actorCollisionInterpolationList_lock.lock();
		if (Globals::actorCollisionInterpolationsList.empty() == false && Globals::actorCollisionInterpolationsList.find(actor->formID) != Globals::actorCollisionInterpolationsList.end())
		{
			LOG_ERR("Actor {} has interpolations", actor->formID);
			tbb::concurrent_unordered_map<std::string, int>& innerMap = Globals::actorCollisionInterpolationsList[actor->formID];

			Globals::actorCollisionInterpolationConfigMap_lock.lock();
			tbb::parallel_for_each(innerMap.begin(), innerMap.end(), [&](const auto& innerPair)
				{
					tbb::concurrent_vector<ConfigLine>* InnerAffectedNodesList = &Globals::collisionInterpolationConfigMap[innerPair.first].AffectedNodesList;
					tbb::parallel_for(static_cast<size_t>(0), InnerAffectedNodesList->size(), [&](size_t i)
						{
							ConfigLine& configLine = InnerAffectedNodesList->at(i);
							if (configLine.NodeName == nodeName)
							{
								LOG_ERR("Applying interpolation for actor: {} - node: {}", actor->formID, nodeName.c_str());
								targetSpheres = configLine.CollisionSpheres;
								for (int j = 0; j < targetSpheres.size(); j++)
								{
									targetSpheres[j].offset0 = GetPointFromPercentage(targetSpheres[j].offset0, targetSpheres[j].offset100, actorWeight);

									targetSpheres[j].radius0 = GetPercentageValue(targetSpheres[j].radius0, targetSpheres[j].radius100, actorWeight);

									targetSpheres[j].radius100pwr2 = targetSpheres[j].radius0 * targetSpheres[j].radius0;
								}

								interpolateSpheres(spheres, targetSpheres, innerPair.second);
							}
						});
				});
			Globals::actorCollisionInterpolationConfigMap_lock.unlock();
		}
		Globals::actorCollisionInterpolationList_lock.unlock();
		return spheres;
	}

	bool areCapsulesEqual(const Capsule& capsule1, const Capsule& capsule2){
		return CompareNiPoints(capsule1.End1_offset0, capsule2.End1_offset0) && CompareNiPoints(capsule1.End2_offset0, capsule2.End2_offset0) && capsule1.End1_radius0 == capsule2.End1_radius0 && capsule1.End2_radius0 == capsule2.End2_radius0;
	}

	Capsule interpolateTwoCapsules(const Capsule& sphere1, const Capsule& sphere2, float percentage) {
		Capsule result;

		result.End1_offset0 = GetPointFromPercentage(sphere1.End1_offset0, sphere2.End1_offset100, percentage);
		result.End1_radius0 = GetPercentageValue(sphere1.End1_radius0, sphere2.End1_radius100, percentage);
		result.End2_offset0 = GetPointFromPercentage(sphere1.End2_offset0, sphere2.End2_offset100, percentage);
		result.End2_radius0 = GetPercentageValue(sphere1.End2_radius0, sphere2.End2_radius100, percentage);
		result.End1_radius100pwr2 = result.End1_radius0 * result.End1_radius0;
		result.End2_radius100pwr2 = result.End2_radius0 * result.End2_radius0;

		return result;
	}

	void interpolateCapsules(std::vector<Capsule>& initialCapsules, const std::vector<Capsule>& targetCapsules, float percentage)
	{
		size_t initialCount = initialCapsules.size();
		size_t targetCount = targetCapsules.size();

		// Interpolate capsules individually
		if (initialCount > 0 && targetCount > 0)
		{
			for (size_t i = 0; i < std::min(initialCount, targetCount); ++i)
			{
				initialCapsules[i] = interpolateTwoCapsules(initialCapsules[i], targetCapsules[i], percentage);
			}
		}
		// If initialCount < targetCount, interpolate towards the last target  after 0.5
		if (initialCount < targetCount && percentage >= 50 && initialCount>0)
		{
			for (size_t i = initialCount; i < targetCount; ++i)
			{
				initialCapsules.push_back(interpolateTwoCapsules(initialCapsules[i % initialCount], targetCapsules[i], 2 * (percentage - 50)));
			}
		}
		// If initialCount > targetCount, duplicate target capsules and interpolate
		else if (initialCount > targetCount && percentage < 50 && targetCount>0)
		{
			for (size_t i = std::min(initialCount, targetCount); i < initialCount; ++i)
			{
				initialCapsules[i] = interpolateTwoCapsules(initialCapsules[i], targetCapsules[i % targetCount], 2 * percentage);
			}
		}

		std::ranges::sort(initialCapsules, [](const Capsule& a, const Capsule& b) {
			if (a.End1_offset0.x != b.End1_offset0.x) return a.End1_offset0.x < b.End1_offset0.x;
			if (a.End1_offset0.y != b.End1_offset0.y) return a.End1_offset0.y < b.End1_offset0.y;
			if (a.End1_offset0.z != b.End1_offset0.z) return a.End1_offset0.z < b.End1_offset0.z;
			if (a.End2_offset0.x != b.End2_offset0.x) return a.End2_offset0.x < b.End2_offset0.x;
			if (a.End2_offset0.y != b.End2_offset0.y) return a.End2_offset0.y < b.End2_offset0.y;
			if (a.End2_offset0.z != b.End2_offset0.z) return a.End2_offset0.z < b.End2_offset0.z;
			if (a.End1_radius0 != b.End1_radius0) return a.End1_radius0 < b.End1_radius0;
			return a.End2_radius0 < b.End2_radius0;
			});

		// Use std::unique to remove adjacent duplicates
		auto last = std::ranges::unique(initialCapsules).begin();

		// Erase the duplicates from the vector
		initialCapsules.erase(last, initialCapsules.end());
	}

	std::vector<Capsule> Thing::CreateThingCollisionCapsules(RE::Actor* actor, const std::string& nodeName)
	{
	

		actorWeight = actor->GetWeight();

		actorBaseScale = actor->GetBaseHeight();
		tbb::concurrent_vector<ConfigLine>* AffectedNodesListPtr;

		std::string actorrefname = "";
		std::string actorRace = "";

		SpecificNPCConfig snc;

		if (actor->formID == 0x14) //If Player
		{
			actorrefname = "Player";
		}
		else
		{
			actorrefname = actor->GetDisplayFullName();
		}

		if (actor->GetRace())
		{
			actorRace = actor->GetRace()->fullName.c_str();
		}

		bool success = GetSpecificNPCConfigForActor(actor, snc);

		if (success)
		{
			AffectedNodesListPtr = &(snc.AffectedNodesList);
			thing_bellybulgemultiplier = snc.cbellybulge;
			thing_bellybulgemax = snc.cbellybulgemax;
			thing_bellybulgeposlowest = snc.cbellybulgeposlowest;
			thing_bellybulgelist = snc.bellybulgenodesList;
			thing_bellybulgeblacklist = snc.bellybulgenodesBlackList;
			thing_bellyBulgeSpeed = snc.bellyBulgeSpeed;
			thing_vaginaOpeningLimit = snc.vaginaOpeningLimit;
			thing_vaginaOpeningMultiplier = snc.vaginaOpeningMultiplier;
			thing_anusOpeningLimit = snc.anusOpeningLimit;
			thing_anusOpeningMultiplier = snc.anusOpeningMultiplier;
		}
		else
		{
			AffectedNodesListPtr = &Globals::AffectedNodesList;
			thing_bellybulgemultiplier = Globals::cbellybulge;
			thing_bellybulgemax = Globals::cbellybulgemax;
			thing_bellybulgeposlowest = Globals::cbellybulgeposlowest;
			thing_bellybulgelist = Globals::bellybulgenodesList;
			thing_bellybulgeblacklist = Globals::bellybulgenodesBlackList;
			thing_bellyBulgeSpeed = Globals::bellyBulgeSpeed;
			thing_vaginaOpeningLimit = Globals::vaginaOpeningLimit;
			thing_vaginaOpeningMultiplier = Globals::vaginaOpeningMultiplier;
			thing_anusOpeningLimit = Globals::anusOpeningLimit;
			thing_anusOpeningMultiplier = Globals::anusOpeningMultiplier;
		}

		std::vector<Capsule> capsules;

		tbb::parallel_for(static_cast<size_t>(0), AffectedNodesListPtr->size(), [&](size_t i)
			{
				if (AffectedNodesListPtr->at(i).NodeName == nodeName)
				{
					capsules = AffectedNodesListPtr->at(i).CollisionCapsules;
					IgnoredCollidersList = AffectedNodesListPtr->at(i).IgnoredColliders;
					IgnoredSelfCollidersList = AffectedNodesListPtr->at(i).IgnoredSelfColliders;
					IgnoreAllSelfColliders = AffectedNodesListPtr->at(i).IgnoreAllSelfColliders;
					for (int j = 0; j < capsules.size(); j++)
					{
						capsules[j].End1_offset0 = GetPointFromPercentage(capsules[j].End1_offset0, capsules[j].End1_offset100, actorWeight);
						capsules[j].End1_radius0 = GetPercentageValue(capsules[j].End1_radius0, capsules[j].End1_radius100, actorWeight);
						capsules[j].End1_radius100pwr2 = capsules[j].End1_radius0 * capsules[j].End1_radius0;
						capsules[j].End2_offset0 = GetPointFromPercentage(capsules[j].End2_offset0, capsules[j].End2_offset100, actorWeight);
						capsules[j].End2_radius0 = GetPercentageValue(capsules[j].End2_radius0, capsules[j].End2_radius100, actorWeight);
						capsules[j].End2_radius100pwr2 = capsules[j].End2_radius0 * capsules[j].End2_radius0;
					}
					scaleWeight = AffectedNodesListPtr->at(i).scaleWeight;
				}
			});

		std::vector<Capsule> targetCapsules;

		Globals::actorCollisionInterpolationList_lock.lock();
		if (Globals::actorCollisionInterpolationsList.empty() == false && Globals::actorCollisionInterpolationsList.find(actor->formID) != Globals::actorCollisionInterpolationsList.end())
		{
			tbb::concurrent_unordered_map<std::string, int>& innerMap = Globals::actorCollisionInterpolationsList[actor->formID];

			Globals::actorCollisionInterpolationConfigMap_lock.lock();
			tbb::parallel_for_each(innerMap.begin(), innerMap.end(), [&](const auto& innerPair)
				{
					tbb::concurrent_vector<ConfigLine>* InnerAffectedNodesList = &Globals::collisionInterpolationConfigMap[innerPair.first].AffectedNodesList;
					tbb::parallel_for(static_cast<size_t>(0), InnerAffectedNodesList->size(), [&](size_t i)
						{
							ConfigLine& configLine = InnerAffectedNodesList->at(i);
							if (configLine.NodeName == nodeName)
							{
								targetCapsules = configLine.CollisionCapsules;
								for (int j = 0; j < targetCapsules.size(); j++)
								{
									targetCapsules[j].End1_offset0 = GetPointFromPercentage(targetCapsules[j].End1_offset0, targetCapsules[j].End1_offset100, actorWeight);
									targetCapsules[j].End1_radius0 = GetPercentageValue(targetCapsules[j].End1_radius0, targetCapsules[j].End1_radius100, actorWeight);
									targetCapsules[j].End1_radius100pwr2 = targetCapsules[j].End1_radius0 * targetCapsules[j].End1_radius0;
									targetCapsules[j].End2_offset0 = GetPointFromPercentage(targetCapsules[j].End2_offset0, targetCapsules[j].End2_offset100, actorWeight);
									targetCapsules[j].End2_radius0 = GetPercentageValue(targetCapsules[j].End2_radius0, targetCapsules[j].End2_radius100, actorWeight);
									targetCapsules[j].End2_radius100pwr2 = targetCapsules[j].End2_radius0 * targetCapsules[j].End2_radius0;
								}

								interpolateCapsules(capsules, targetCapsules, innerPair.second);
							}
						});
				});
			Globals::actorCollisionInterpolationConfigMap_lock.unlock();
		}
		Globals::actorCollisionInterpolationList_lock.unlock();
		return capsules;
	}

	void showPos(RE::NiPoint3& p) {
		LOG_INFO("%8.2f %8.2f %8.2f\n", p.x, p.y, p.z);
	}

	void showRot(RE::NiMatrix3& r) {
		LOG_INFO("%8.2f %8.2f %8.2f\n", r.entry[0][0], r.entry[0][1], r.entry[0][2]);
		LOG_INFO("%8.2f %8.2f %8.2f\n", r.entry[1][0], r.entry[1][1], r.entry[1][2]);
		LOG_INFO("%8.2f %8.2f %8.2f\n", r.entry[2][0], r.entry[2][1], r.entry[2][2]);
	}


	float solveQuad(float a, float b, float c) {
		float k1 = (-b + sqrtf(b * b - 4 * a * c)) / (2 * a);
		//float k2 = (-b - sqrtf(b*b - 4*a*c)) / (2 * a);
		//logger.error("k2 = {}\n", k2);
		return k1;
	}

	void Thing::updateConfigValues(RE::Actor* actor)
	{
		stiffness = GetPercentageValue(stiffness_0, stiffness_100, actorWeight);
		stiffnessX = GetPercentageValue(stiffnessX_0, stiffnessX_100, actorWeight);
		stiffnessY = GetPercentageValue(stiffnessY_0, stiffnessY_100, actorWeight);
		stiffnessZ = GetPercentageValue(stiffnessZ_0, stiffnessZ_100, actorWeight);
		stiffnessXRot = GetPercentageValue(stiffnessXRot_0, stiffnessXRot_100, actorWeight);
		stiffnessYRot = GetPercentageValue(stiffnessYRot_0, stiffnessYRot_100, actorWeight);
		stiffnessZRot = GetPercentageValue(stiffnessZRot_0, stiffnessZRot_100, actorWeight);
		stiffness2 = GetPercentageValue(stiffness2_0, stiffness2_100, actorWeight);
		stiffness2X = GetPercentageValue(stiffness2X_0, stiffness2X_100, actorWeight);
		stiffness2Y = GetPercentageValue(stiffness2Y_0, stiffness2Y_100, actorWeight);
		stiffness2Z = GetPercentageValue(stiffness2Z_0, stiffness2Z_100, actorWeight);
		stiffness2XRot = GetPercentageValue(stiffness2XRot_0, stiffness2XRot_100, actorWeight);
		stiffness2YRot = GetPercentageValue(stiffness2YRot_0, stiffness2YRot_100, actorWeight);
		stiffness2ZRot = GetPercentageValue(stiffness2ZRot_0, stiffness2ZRot_100, actorWeight);
		damping = GetPercentageValue(damping_0, damping_100, actorWeight);
		dampingX = GetPercentageValue(dampingX_0, dampingX_100, actorWeight);
		dampingY = GetPercentageValue(dampingY_0, dampingY_100, actorWeight);
		dampingZ = GetPercentageValue(dampingZ_0, dampingZ_100, actorWeight);
		dampingXRot = GetPercentageValue(dampingXRot_0, dampingXRot_100, actorWeight);
		dampingYRot = GetPercentageValue(dampingYRot_0, dampingYRot_100, actorWeight);
		dampingZRot = GetPercentageValue(dampingZRot_0, dampingZRot_100, actorWeight);

		//maxOffset = GetPercentageValue(maxOffset_0, maxOffset_100, actorWeight);
		XmaxOffset = GetPercentageValue(XmaxOffset_0, XmaxOffset_100, actorWeight);
		XminOffset = GetPercentageValue(XminOffset_0, XminOffset_100, actorWeight);
		YmaxOffset = GetPercentageValue(YmaxOffset_0, YmaxOffset_100, actorWeight);
		YminOffset = GetPercentageValue(YminOffset_0, YminOffset_100, actorWeight);
		ZmaxOffset = GetPercentageValue(ZmaxOffset_0, ZmaxOffset_100, actorWeight);
		ZminOffset = GetPercentageValue(ZminOffset_0, ZminOffset_100, actorWeight);
		XmaxOffsetRot = GetPercentageValue(XmaxOffsetRot_0, XmaxOffsetRot_100, actorWeight);
		XminOffsetRot = GetPercentageValue(XminOffsetRot_0, XminOffsetRot_100, actorWeight);
		YmaxOffsetRot = GetPercentageValue(YmaxOffsetRot_0, YmaxOffsetRot_100, actorWeight);
		YminOffsetRot = GetPercentageValue(YminOffsetRot_0, YminOffsetRot_100, actorWeight);
		ZmaxOffsetRot = GetPercentageValue(ZmaxOffsetRot_0, ZmaxOffsetRot_100, actorWeight);
		ZminOffsetRot = GetPercentageValue(ZminOffsetRot_0, ZminOffsetRot_100, actorWeight);
		XdefaultOffset = GetPercentageValue(XdefaultOffset_0, XdefaultOffset_100, actorWeight);
		YdefaultOffset = GetPercentageValue(YdefaultOffset_0, YdefaultOffset_100, actorWeight);
		ZdefaultOffset = GetPercentageValue(ZdefaultOffset_0, ZdefaultOffset_100, actorWeight);
		cogOffset = GetPercentageValue(cogOffset_0, cogOffset_100, actorWeight);
		gravityBias = GetPercentageValue(gravityBias_0, gravityBias_100, actorWeight);
		gravityCorrection = GetPercentageValue(gravityCorrection_0, gravityCorrection_100, actorWeight);
		varGravityCorrection = -1 * gravityCorrection;

		timeTick = GetPercentageValue(timeTick_0, timeTick_100, actorWeight);
		timeTickRot = GetPercentageValue(timeTickRot_0, timeTickRot_100, actorWeight);
		linearX = GetPercentageValue(linearX_0, linearX_100, actorWeight);
		linearY = GetPercentageValue(linearY_0, linearY_100, actorWeight);
		linearZ = GetPercentageValue(linearZ_0, linearZ_100, actorWeight);
		rotationalXnew = GetPercentageValue(rotationalXnew_0, rotationalXnew_100, actorWeight);
		rotationalYnew = GetPercentageValue(rotationalYnew_0, rotationalYnew_100, actorWeight);
		rotationalZnew = GetPercentageValue(rotationalZnew_0, rotationalZnew_100, actorWeight);
		linearXrotationX = GetPercentageValue(linearXrotationX_0, linearXrotationX_100, actorWeight);
		linearXrotationY = GetPercentageValue(linearXrotationY_0, linearXrotationY_100, actorWeight);
		linearXrotationZ = GetPercentageValue(linearXrotationZ_0, linearXrotationZ_100, actorWeight);
		linearYrotationX = GetPercentageValue(linearYrotationX_0, linearYrotationX_100, actorWeight);
		linearYrotationY = GetPercentageValue(linearYrotationY_0, linearYrotationY_100, actorWeight);
		linearYrotationZ = GetPercentageValue(linearYrotationZ_0, linearYrotationZ_100, actorWeight);
		linearZrotationX = GetPercentageValue(linearZrotationX_0, linearZrotationX_100, actorWeight);
		linearZrotationY = GetPercentageValue(linearZrotationY_0, linearZrotationY_100, actorWeight);
		linearZrotationZ = GetPercentageValue(linearZrotationZ_0, linearZrotationZ_100, actorWeight);
		timeStep = GetPercentageValue(timeStep_0, timeStep_100, actorWeight);
		timeStepRot = GetPercentageValue(timeStepRot_0, timeStepRot_100, actorWeight);

		linearXspreadforceY = GetPercentageValue(linearXspreadforceY_0, linearXspreadforceY_100, actorWeight);
		linearXspreadforceZ = GetPercentageValue(linearXspreadforceZ_0, linearXspreadforceZ_100, actorWeight);
		linearYspreadforceX = GetPercentageValue(linearYspreadforceX_0, linearYspreadforceX_100, actorWeight);
		linearYspreadforceZ = GetPercentageValue(linearYspreadforceZ_0, linearYspreadforceZ_100, actorWeight);
		linearZspreadforceX = GetPercentageValue(linearZspreadforceX_0, linearZspreadforceX_100, actorWeight);
		linearZspreadforceY = GetPercentageValue(linearZspreadforceY_0, linearZspreadforceY_100, actorWeight);
		rotationXspreadforceY = GetPercentageValue(rotationXspreadforceY_0, rotationXspreadforceY_100, actorWeight);
		rotationXspreadforceZ = GetPercentageValue(rotationXspreadforceZ_0, rotationXspreadforceZ_100, actorWeight);
		rotationYspreadforceX = GetPercentageValue(rotationYspreadforceX_0, rotationYspreadforceX_100, actorWeight);
		rotationYspreadforceZ = GetPercentageValue(rotationYspreadforceZ_0, rotationYspreadforceZ_100, actorWeight);
		rotationZspreadforceX = GetPercentageValue(rotationZspreadforceX_0, rotationZspreadforceX_100, actorWeight);
		rotationZspreadforceY = GetPercentageValue(rotationZspreadforceY_0, rotationZspreadforceY_100, actorWeight);

		forceMultipler = GetPercentageValue(forceMultipler_0, forceMultipler_100, actorWeight);

		gravityInvertedCorrection = GetPercentageValue(gravityInvertedCorrection_0, gravityInvertedCorrection_100, actorWeight);
		gravityInvertedCorrectionStart = GetPercentageValue(gravityInvertedCorrectionStart_0, gravityInvertedCorrectionStart_100, actorWeight);
		gravityInvertedCorrectionEnd = GetPercentageValue(gravityInvertedCorrectionEnd_0, gravityInvertedCorrectionEnd_100, actorWeight);

		breastClothedPushup = GetPercentageValue(breastClothedPushup_0, breastClothedPushup_100, actorWeight);
		breastLightArmoredPushup = GetPercentageValue(breastLightArmoredPushup_0, breastLightArmoredPushup_100, actorWeight);
		breastHeavyArmoredPushup = GetPercentageValue(breastHeavyArmoredPushup_0, breastHeavyArmoredPushup_100, actorWeight);

		breastClothedAmplitude = GetPercentageValue(breastClothedAmplitude_0, breastClothedAmplitude_100, actorWeight);
		breastLightArmoredAmplitude = GetPercentageValue(breastLightArmoredAmplitude_0, breastLightArmoredAmplitude_100, actorWeight);
		breastHeavyArmoredAmplitude = GetPercentageValue(breastHeavyArmoredAmplitude_0, breastHeavyArmoredAmplitude_100, actorWeight);

		collisionFriction = GetPercentageValue(collisionFriction_0, collisionFriction_100, actorWeight);
		collisionPenetration = GetPercentageValue(collisionPenetration_0, collisionPenetration_100, actorWeight);
		collisionMultipler = GetPercentageValue(collisionMultipler_0, collisionMultipler_100, actorWeight);
		if (collisionMultipler >= 1.01f || collisionMultipler <= 0.99f)
			VirtualCollisionEnabled = true;
		collisionMultiplerRot = GetPercentageValue(collisionMultiplerRot_0, collisionMultiplerRot_100, actorWeight);

		float collisionReactionValue = GetPercentageValue(collisionElastic_0, collisionElastic_100, actorWeight);
		if (collisionReactionValue > 0.5f)
			collisionElastic = true;
		else
			collisionElastic = false;

		float collisionElasticConstraintsValue = GetPercentageValue(collisionElasticConstraints_0, collisionElasticConstraints_100, actorWeight);
		if (collisionElasticConstraintsValue > 0.5f)
			collisionElasticConstraints = true;
		else
			collisionElasticConstraints = false;


		collisionXmaxOffset = GetPercentageValue(collisionXmaxOffset_0, collisionXmaxOffset_100, actorWeight);
		collisionXminOffset = GetPercentageValue(collisionXminOffset_0, collisionXminOffset_100, actorWeight);
		collisionYmaxOffset = GetPercentageValue(collisionYmaxOffset_0, collisionYmaxOffset_100, actorWeight);
		collisionYminOffset = GetPercentageValue(collisionYminOffset_0, collisionYminOffset_100, actorWeight);
		collisionZmaxOffset = GetPercentageValue(collisionZmaxOffset_0, collisionZmaxOffset_100, actorWeight);
		collisionZminOffset = GetPercentageValue(collisionZminOffset_0, collisionZminOffset_100, actorWeight);

		amplitude = GetPercentageValue(amplitude_0, amplitude_100, actorWeight);

		CollisionConfig.IsElasticCollision = collisionElastic;

		CollisionConfig.CollisionMaxOffset = RE::NiPoint3(collisionXmaxOffset, collisionYmaxOffset, collisionZmaxOffset);
		CollisionConfig.CollisionMinOffset = RE::NiPoint3(collisionXminOffset, collisionYminOffset, collisionZminOffset);

	}

	void Thing::updateConfig(RE::Actor* actor, configEntry_t centry, configEntry_t centry0weight, std::string section)
	{
		if (actor != nullptr)
		{
			Globals::actorBounceInterpolationList_lock.lock();
			if (Globals::actorBounceInterpolationsList.empty() == false && Globals::actorBounceInterpolationsList.find(actor->formID) != Globals::actorBounceInterpolationsList.end())
			{
				tbb::concurrent_unordered_map<std::string, int>& innerMap = Globals::actorBounceInterpolationsList[actor->formID];

				Globals::actorBounceInterpolationConfigMap_lock.lock();

				for (auto& innerPair : innerMap)
				{
					for (auto& eachSetting : Globals::bounceInterpolationConfigMap[innerPair.first].config[section])
					{
						const float oldValue = centry[eachSetting.first];
						centry[eachSetting.first] = GetPercentageValue(centry[eachSetting.first], eachSetting.second, innerPair.second);
						LOG_ERR("Bounce100 {}: {} -> {} for {}'s {}", eachSetting.first.c_str(), oldValue, centry[eachSetting.first], actor->formID, section.c_str());
					}
					for (auto& eachSetting : Globals::bounceInterpolationConfigMap[innerPair.first].config0weight[section])
					{
						const float oldValue = centry0weight[eachSetting.first];
						centry0weight[eachSetting.first] = GetPercentageValue(centry0weight[eachSetting.first], eachSetting.second, innerPair.second);
						LOG_ERR("Bounce0 {}: {} -> {} for {}'s {}", eachSetting.first.c_str(), oldValue, centry0weight[eachSetting.first], actor->formID, section.c_str());
					}
				}
				Globals::actorBounceInterpolationConfigMap_lock.unlock();
			}
			Globals::actorBounceInterpolationList_lock.unlock();
		}
		//100 weight	
		stiffness_100 = centry["stiffness"];
		stiffnessX_100 = centry["stiffnessX"];
		stiffnessY_100 = centry["stiffnessY"];
		stiffnessZ_100 = centry["stiffnessZ"];
		if (stiffness_100 >= 0.001f && stiffnessX_100 < 0.001f && stiffnessY_100 < 0.001f && stiffnessZ_100 < 0.001f)
		{
			stiffnessX_100 = stiffness_100;
			stiffnessY_100 = stiffness_100;
			stiffnessZ_100 = stiffness_100;
		}
		stiffnessXRot_100 = centry["stiffnessXRot"];
		stiffnessYRot_100 = centry["stiffnessYRot"];
		stiffnessZRot_100 = centry["stiffnessZRot"];
		if (stiffness_100 >= 0.001f && stiffnessXRot_100 < 0.001f && stiffnessYRot_100 < 0.001f && stiffnessZRot_100 < 0.001f)
		{
			stiffnessXRot_100 = stiffness_100;
			stiffnessYRot_100 = stiffness_100;
			stiffnessZRot_100 = stiffness_100;
		}
		stiffness2_100 = centry["stiffness2"];
		stiffness2X_100 = centry["stiffness2X"];
		stiffness2Y_100 = centry["stiffness2Y"];
		stiffness2Z_100 = centry["stiffness2Z"];
		if (stiffness2_100 >= 0.001f && stiffness2X_100 < 0.001f && stiffness2Y_100 < 0.001f && stiffness2Z_100 < 0.001f)
		{
			stiffness2X_100 = stiffness2_100;
			stiffness2Y_100 = stiffness2_100;
			stiffness2Z_100 = stiffness2_100;
		}
		stiffness2XRot_100 = centry["stiffness2XRot"];
		stiffness2YRot_100 = centry["stiffness2YRot"];
		stiffness2ZRot_100 = centry["stiffness2ZRot"];
		if (stiffness2_100 >= 0.001f && stiffness2XRot_100 < 0.001f && stiffness2YRot_100 < 0.001f && stiffness2ZRot_100 < 0.001f)
		{
			stiffness2XRot_100 = stiffness2_100;
			stiffness2YRot_100 = stiffness2_100;
			stiffness2ZRot_100 = stiffness2_100;
		}
		damping_100 = centry["damping"];
		dampingX_100 = centry["dampingX"];
		dampingY_100 = centry["dampingY"];
		dampingZ_100 = centry["dampingZ"];
		if (damping_100 >= 0.001f && dampingX_100 < 0.001f && dampingY_100 < 0.001f && dampingZ_100 < 0.001f)
		{
			dampingX_100 = damping_100;
			dampingY_100 = damping_100;
			dampingZ_100 = damping_100;
		}
		dampingXRot_100 = centry["dampingXRot"];
		dampingYRot_100 = centry["dampingYRot"];
		dampingZRot_100 = centry["dampingZRot"];
		if (dampingXRot_100 < 0.001f && dampingYRot_100 < 0.001f && dampingZRot_100 < 0.001f)
		{
			dampingXRot_100 = dampingX_100;
			dampingYRot_100 = dampingY_100;
			dampingZRot_100 = dampingZ_100;
		}
		maxOffset_100 = centry["maxoffset"];
		if (maxOffset_100 >= 0.01f)
		{
			XmaxOffset_100 = maxOffset_100;
			XminOffset_100 = -maxOffset_100;
			YmaxOffset_100 = maxOffset_100;
			YminOffset_100 = -maxOffset_100;
			ZmaxOffset_100 = maxOffset_100;
			ZminOffset_100 = -maxOffset_100;
			XmaxOffsetRot_100 = maxOffset_100;
			XminOffsetRot_100 = -maxOffset_100;
			YmaxOffsetRot_100 = maxOffset_100;
			YminOffsetRot_100 = -maxOffset_100;
			ZmaxOffsetRot_100 = maxOffset_100;
			ZminOffsetRot_100 = -maxOffset_100;
		}
		else
		{
			XmaxOffset_100 = centry["Xmaxoffset"];
			XminOffset_100 = centry["Xminoffset"];
			YmaxOffset_100 = centry["Ymaxoffset"];
			YminOffset_100 = centry["Yminoffset"];
			ZmaxOffset_100 = centry["Zmaxoffset"];
			ZminOffset_100 = centry["Zminoffset"];
			XmaxOffsetRot_100 = centry["XmaxoffsetRot"];
			XminOffsetRot_100 = centry["XminoffsetRot"];
			YmaxOffsetRot_100 = centry["YmaxoffsetRot"];
			YminOffsetRot_100 = centry["YminoffsetRot"];
			ZmaxOffsetRot_100 = centry["ZmaxoffsetRot"];
			ZminOffsetRot_100 = centry["ZminoffsetRot"];
			if (XmaxOffsetRot_100 < 0.001f && XminOffsetRot_100 < 0.001f && YmaxOffsetRot_100 < 0.001f && YminOffsetRot_100 < 0.001f && ZmaxOffsetRot_100 < 0.001f && ZminOffsetRot_100 < 0.001f)
			{
				XmaxOffsetRot_100 = ZmaxOffset_100;
				XminOffsetRot_100 = ZminOffset_100;
				YmaxOffsetRot_100 = XmaxOffset_100;
				YminOffsetRot_100 = XminOffset_100;
				ZmaxOffsetRot_100 = YmaxOffset_100;
				ZminOffsetRot_100 = YminOffset_100;
			}
		}
		XdefaultOffset_100 = centry["Xdefaultoffset_100"];
		YdefaultOffset_100 = centry["Ydefaultoffset_100"];
		ZdefaultOffset_100 = centry["Zdefaultoffset_100"];

		timeTick_100 = centry["timetick"];
		if (timeTick_100 <= 1.0f)
			timeTick_100 = 1.0f;
		timeTickRot_100 = centry["timetickRot"];
		if (timeTickRot_100 < 1.0f)
			timeTickRot_100 = timeTick_100;
		linearX_100 = centry["linearX"];
		linearY_100 = centry["linearY"];
		linearZ_100 = centry["linearZ"];

		rotationalXnew_100 = centry["rotational"];
		if (rotationalXnew_100 < 0.0001f)
		{
			rotationalXnew_100 = centry["rotationalX"];
		}
		rotationalYnew_100 = centry["rotationalY"];
		rotationalZnew_100 = centry["rotationalZ"];

		linearXrotationX_100 = centry["linearXrotationX"];
		linearXrotationY_100 = centry["linearXrotationY"];
		linearXrotationZ_100 = centry["linearXrotationZ"];
		linearYrotationX_100 = centry["linearYrotationX"];
		linearYrotationY_100 = centry["linearYrotationY"];
		linearYrotationZ_100 = centry["linearYrotationZ"];
		linearZrotationX_100 = centry["linearZrotationX"];
		linearZrotationY_100 = centry["linearZrotationY"];
		linearZrotationZ_100 = centry["linearZrotationZ"];

		timeStep_100 = centry["timeStep"];
		timeStepRot_100 = centry["timeStepRot"];
		if (timeStepRot_100 < 0.001f)
			timeStepRot_100 = timeStep_100;

		linearXspreadforceY_100 = centry["linearXspreadforceY"];
		linearXspreadforceZ_100 = centry["linearXspreadforceZ"];
		linearYspreadforceX_100 = centry["linearYspreadforceX"];
		linearYspreadforceZ_100 = centry["linearYspreadforceZ"];
		linearZspreadforceX_100 = centry["linearZspreadforceX"];
		linearZspreadforceY_100 = centry["linearZspreadforceY"];
		rotationXspreadforceY_100 = centry["rotationXspreadforceY"];
		rotationXspreadforceZ_100 = centry["rotationXspreadforceZ"];
		rotationYspreadforceX_100 = centry["rotationYspreadforceX"];
		rotationYspreadforceZ_100 = centry["rotationYspreadforceZ"];
		rotationZspreadforceX_100 = centry["rotationZspreadforceX"];
		rotationZspreadforceY_100 = centry["rotationZspreadforceY"];

		forceMultipler_100 = centry["forceMultipler"];

		gravityBias_100 = centry["gravityBias"];
		gravityCorrection_100 = centry["gravityCorrection"];
		cogOffset_100 = centry["cogOffset"];

		gravityInvertedCorrection_100 = centry["gravityInvertedCorrection"];
		gravityInvertedCorrectionStart_100 = centry["gravityInvertedCorrectionStart"];
		gravityInvertedCorrectionEnd_100 = centry["gravityInvertedCorrectionEnd"];

		breastClothedPushup_100 = centry["breastClothedPushup"];
		breastLightArmoredPushup_100 = centry["breastLightArmoredPushup"];
		breastHeavyArmoredPushup_100 = centry["breastHeavyArmoredPushup"];

		breastClothedAmplitude_100 = centry["breastClothedAmplitude"];
		breastLightArmoredAmplitude_100 = centry["breastLightArmoredAmplitude"];
		breastHeavyArmoredAmplitude_100 = centry["breastHeavyArmoredAmplitude"];

		collisionFriction_100 = 1.0f - centry["collisionFriction"];
		if (collisionFriction_100 < 0.0f)
			collisionFriction_100 = 0.0f;
		else if (collisionFriction_100 > 1.0f)
			collisionFriction_100 = 1.0f;

		collisionPenetration_100 = 1.0f - centry["collisionPenetration"];
		if (collisionPenetration_100 < 0.0f)
			collisionPenetration_100 = 0.0f;
		else if (collisionPenetration_100 > 1.0f)
			collisionPenetration_100 = 1.0f;

		collisionMultipler_100 = centry["collisionMultipler"];

		collisionMultiplerRot_100 = centry["collisionMultiplerRot"];
		collisionElastic_100 = centry["collisionElastic"];
		collisionElasticConstraints_100 = centry["collisionElasticConstraints"];

		collisionXmaxOffset_100 = centry["collisionXmaxoffset"];
		collisionXminOffset_100 = centry["collisionXminoffset"];
		collisionYmaxOffset_100 = centry["collisionYmaxoffset"];
		collisionYminOffset_100 = centry["collisionYminoffset"];
		collisionZmaxOffset_100 = centry["collisionZmaxoffset"];
		collisionZminOffset_100 = centry["collisionZminoffset"];

		amplitude_100 = centry["amplitude"];


		//0 weight
		stiffness_0 = centry0weight["stiffness"];
		stiffnessX_0 = centry0weight["stiffnessX"];
		stiffnessY_0 = centry0weight["stiffnessY"];
		stiffnessZ_0 = centry0weight["stiffnessZ"];
		if (stiffness_0 >= 0.001f && stiffnessX_0 < 0.001f && stiffnessY_0 < 0.001f && stiffnessZ_0 < 0.001f)
		{
			stiffnessX_0 = stiffness_0;
			stiffnessY_0 = stiffness_0;
			stiffnessZ_0 = stiffness_0;
		}
		stiffnessXRot_0 = centry0weight["stiffnessXRot"];
		stiffnessYRot_0 = centry0weight["stiffnessYRot"];
		stiffnessZRot_0 = centry0weight["stiffnessZRot"];
		if (stiffness_0 >= 0.001f && stiffnessXRot_0 < 0.001f && stiffnessYRot_0 < 0.001f && stiffnessZRot_0 < 0.001f)
		{
			stiffnessXRot_0 = stiffness_0;
			stiffnessYRot_0 = stiffness_0;
			stiffnessZRot_0 = stiffness_0;
		}
		stiffness2_0 = centry0weight["stiffness2"];
		stiffness2X_0 = centry0weight["stiffness2X"];
		stiffness2Y_0 = centry0weight["stiffness2Y"];
		stiffness2Z_0 = centry0weight["stiffness2Z"];
		if (stiffness2_0 >= 0.001f && stiffness2X_0 < 0.001f && stiffness2Y_0 < 0.001f && stiffness2Z_0 < 0.001f)
		{
			stiffness2X_0 = stiffness2_0;
			stiffness2Y_0 = stiffness2_0;
			stiffness2Z_0 = stiffness2_0;
		}
		stiffness2XRot_0 = centry0weight["stiffness2XRot"];
		stiffness2YRot_0 = centry0weight["stiffness2YRot"];
		stiffness2ZRot_0 = centry0weight["stiffness2ZRot"];
		if (stiffness2_0 >= 0.001f && stiffness2XRot_0 < 0.001f && stiffness2YRot_0 < 0.001f && stiffness2ZRot_0 < 0.001f)
		{
			stiffness2XRot_0 = stiffness2_0;
			stiffness2YRot_0 = stiffness2_0;
			stiffness2ZRot_0 = stiffness2_0;
		}
		damping_0 = centry0weight["damping"];
		dampingX_0 = centry0weight["dampingX"];
		dampingY_0 = centry0weight["dampingY"];
		dampingZ_0 = centry0weight["dampingZ"];
		if (damping_0 >= 0.001f && dampingX_0 < 0.001f && dampingY_0 < 0.001f && dampingZ_0 < 0.001f)
		{
			dampingX_0 = damping_0;
			dampingY_0 = damping_0;
			dampingZ_0 = damping_0;
		}
		dampingXRot_0 = centry0weight["dampingXRot"];
		dampingYRot_0 = centry0weight["dampingYRot"];
		dampingZRot_0 = centry0weight["dampingZRot"];
		if (damping_0 >= 0.001f && dampingXRot_0 < 0.001f && dampingYRot_0 < 0.001f && dampingZRot_0 < 0.001f)
		{
			dampingXRot_0 = damping_0;
			dampingYRot_0 = damping_0;
			dampingZRot_0 = damping_0;
		}
		maxOffset_0 = centry0weight["maxoffset"];
		if (maxOffset_0 >= 0.01f)
		{
			XmaxOffset_0 = maxOffset_0;
			XminOffset_0 = -maxOffset_0;
			YmaxOffset_0 = maxOffset_0;
			YminOffset_0 = -maxOffset_0;
			ZmaxOffset_0 = maxOffset_0;
			ZminOffset_0 = -maxOffset_0;
			XmaxOffsetRot_0 = maxOffset_0;
			XminOffsetRot_0 = -maxOffset_0;
			YmaxOffsetRot_0 = maxOffset_0;
			YminOffsetRot_0 = -maxOffset_0;
			ZmaxOffsetRot_0 = maxOffset_0;
			ZminOffsetRot_0 = -maxOffset_0;
		}
		else
		{
			XmaxOffset_0 = centry0weight["Xmaxoffset"];
			XminOffset_0 = centry0weight["Xminoffset"];
			YmaxOffset_0 = centry0weight["Ymaxoffset"];
			YminOffset_0 = centry0weight["Yminoffset"];
			ZmaxOffset_0 = centry0weight["Zmaxoffset"];
			ZminOffset_0 = centry0weight["Zminoffset"];
			XmaxOffsetRot_0 = centry0weight["XmaxoffsetRot"];
			XminOffsetRot_0 = centry0weight["XminoffsetRot"];
			YmaxOffsetRot_0 = centry0weight["YmaxoffsetRot"];
			YminOffsetRot_0 = centry0weight["YminoffsetRot"];
			ZmaxOffsetRot_0 = centry0weight["ZmaxoffsetRot"];
			ZminOffsetRot_0 = centry0weight["ZminoffsetRot"];
			if (XmaxOffsetRot_0 < 0.001f && XminOffsetRot_0 < 0.001f && YmaxOffsetRot_0 < 0.001f && YminOffsetRot_0 < 0.001f && ZmaxOffsetRot_0 < 0.001f && ZminOffsetRot_0 < 0.001f)
			{
				XmaxOffsetRot_0 = ZmaxOffset_0;
				XminOffsetRot_0 = ZminOffset_0;
				YmaxOffsetRot_0 = XmaxOffset_0;
				YminOffsetRot_0 = XminOffset_0;
				ZmaxOffsetRot_0 = YmaxOffset_0;
				ZminOffsetRot_0 = YminOffset_0;
			}
		}
		XdefaultOffset_0 = centry0weight["Xdefaultoffset_0"];
		YdefaultOffset_0 = centry0weight["Ydefaultoffset_0"];
		ZdefaultOffset_0 = centry0weight["Zdefaultoffset_0"];

		timeTick_0 = centry0weight["timetick"];
		if (timeTick_0 <= 1.0f)
			timeTick_0 = 1.0f;
		timeTickRot_0 = centry0weight["timetickRot"];
		if (timeTickRot_0 < 1.0f)
			timeTickRot_0 = timeTick_0;
		linearX_0 = centry0weight["linearX"];
		linearY_0 = centry0weight["linearY"];
		linearZ_0 = centry0weight["linearZ"];

		rotationalXnew_0 = centry0weight["rotational"];
		if (rotationalXnew_0 < 0.0001f)
		{
			rotationalXnew_0 = centry0weight["rotationalX"];
		}
		rotationalYnew_0 = centry0weight["rotationalY"];
		rotationalZnew_0 = centry0weight["rotationalZ"];

		linearXrotationX_0 = centry0weight["linearXrotationX"];
		linearXrotationY_0 = centry0weight["linearXrotationY"];
		linearXrotationZ_0 = centry0weight["linearXrotationZ"];
		linearYrotationX_0 = centry0weight["linearYrotationX"];
		linearYrotationY_0 = centry0weight["linearYrotationY"];
		linearYrotationZ_0 = centry0weight["linearYrotationZ"];
		linearZrotationX_0 = centry0weight["linearZrotationX"];
		linearZrotationY_0 = centry0weight["linearZrotationY"];
		linearZrotationZ_0 = centry0weight["linearZrotationZ"];

		timeStep_0 = centry0weight["timeStep"];
		timeStepRot_0 = centry0weight["timeStepRot"];
		if (timeStepRot_0 < 0.001f)
			timeStepRot_0 = timeStep_0;

		linearXspreadforceY_0 = centry0weight["linearXspreadforceY"];
		linearXspreadforceZ_0 = centry0weight["linearXspreadforceZ"];
		linearYspreadforceX_0 = centry0weight["linearYspreadforceX"];
		linearYspreadforceZ_0 = centry0weight["linearYspreadforceZ"];
		linearZspreadforceX_0 = centry0weight["linearZspreadforceX"];
		linearZspreadforceY_0 = centry0weight["linearZspreadforceY"];
		rotationXspreadforceY_0 = centry0weight["rotationXspreadforceY"];
		rotationXspreadforceZ_0 = centry0weight["rotationXspreadforceZ"];
		rotationYspreadforceX_0 = centry0weight["rotationYspreadforceX"];
		rotationYspreadforceZ_0 = centry0weight["rotationYspreadforceZ"];
		rotationZspreadforceX_0 = centry0weight["rotationZspreadforceX"];
		rotationZspreadforceY_0 = centry0weight["rotationZspreadforceY"];

		forceMultipler_0 = centry0weight["forceMultipler"];

		gravityBias_0 = centry0weight["gravityBias"];
		gravityCorrection_0 = centry0weight["gravityCorrection"];
		cogOffset_0 = centry0weight["cogOffset"];

		gravityInvertedCorrection_0 = centry0weight["gravityInvertedCorrection"];
		gravityInvertedCorrectionStart_0 = centry0weight["gravityInvertedCorrectionStart"];
		gravityInvertedCorrectionEnd_0 = centry0weight["gravityInvertedCorrectionEnd"];

		breastClothedPushup_0 = centry0weight["breastClothedPushup"];
		breastLightArmoredPushup_0 = centry0weight["breastLightArmoredPushup"];
		breastHeavyArmoredPushup_0 = centry0weight["breastHeavyArmoredPushup"];

		breastClothedAmplitude_0 = centry0weight["breastClothedAmplitude"];
		breastLightArmoredAmplitude_0 = centry0weight["breastLightArmoredAmplitude"];
		breastHeavyArmoredAmplitude_0 = centry0weight["breastHeavyArmoredAmplitude"];

		collisionFriction_0 = 1.0f - centry0weight["collisionFriction"];
		if (collisionFriction_0 < 0.0f)
			collisionFriction_0 = 0.0f;
		else if (collisionFriction_0 > 1.0f)
			collisionFriction_0 = 1.0f;

		collisionPenetration_0 = 1.0f - centry0weight["collisionPenetration"];
		if (collisionPenetration_0 < 0.0f)
			collisionPenetration_0 = 0.0f;
		else if (collisionPenetration_0 > 1.0f)
			collisionPenetration_0 = 1.0f;

		collisionMultipler_0 = centry0weight["collisionMultipler"];
		collisionMultiplerRot_0 = centry0weight["collisionMultiplerRot"];

		collisionElastic_0 = centry0weight["collisionElastic"];
		collisionElasticConstraints_0 = centry0weight["collisionElasticConstraints"];

		collisionXmaxOffset_0 = centry0weight["collisionXmaxoffset"];
		collisionXminOffset_0 = centry0weight["collisionXminoffset"];
		collisionYmaxOffset_0 = centry0weight["collisionYmaxoffset"];
		collisionYminOffset_0 = centry0weight["collisionYminoffset"];
		collisionZmaxOffset_0 = centry0weight["collisionZmaxoffset"];
		collisionZminOffset_0 = centry0weight["collisionZminoffset"];

		amplitude_0 = centry0weight["amplitude"];

		updateConfigValues(actor);

		//zOffset = solveQuad(stiffness2, stiffness, -gravityBias);

		//logger.error("z offset = {}\n", solveQuad(stiffness2, stiffness, -gravityBias));
	}

	void Thing::dump() {
		//showPos(obj->world.translate);
		//showPos(obj->local.translate);
	}

	void Thing::reset() {
		// TODO
	}

	template <typename T> int sgn(T val) {
		return (T(0) < val) - (val < T(0));
	}

	void Thing::updatePelvis(RE::Actor* actor, std::shared_mutex& thing_ReadNode_lock, std::shared_mutex& thing_SetNode_lock)
	{
		if (skipFramesPelvisCount > 0)
		{
			skipFramesPelvisCount--;
			return;
		}
		else
		{
			skipFramesPelvisCount = Globals::collisionSkipFramesPelvis;
		}


		if (!RE::PlayerCharacter::GetSingleton()->Is3DLoaded()){
			return;
		}

		auto loadedState = actor->loadedData;
		if (!loadedState || !loadedState->data3D){
			return;
		}

		thing_ReadNode_lock.lock();
		RE::NiAVObject* leftPusObj = loadedState->data3D->GetObjectByName(Globals::leftPus.c_str());
		RE::NiAVObject* rightPusObj = loadedState->data3D->GetObjectByName(Globals::rightPus.c_str());
		RE::NiAVObject* backPusObj = loadedState->data3D->GetObjectByName(Globals::backPus.c_str());
		RE::NiAVObject* frontPusObj = loadedState->data3D->GetObjectByName(Globals::frontPus.c_str());
		RE::NiAVObject* pelvisObj = loadedState->data3D->GetObjectByName(Globals::pelvis);
		thing_ReadNode_lock.unlock();

		if (!leftPusObj || !rightPusObj || !backPusObj || !frontPusObj || !pelvisObj)
			return;

		if (updatePussyFirstRun)
		{
			updatePussyFirstRun = false;

			auto leftpair = std::make_pair(actor->GetActorBase()->formID, Globals::leftPus.c_str());
			thing_map_lock.lock();
			std::map<std::pair<unsigned, std::string_view>, RE::NiPoint3>::iterator posMap = Globals::thingDefaultPosList.find(leftpair);

			if (posMap == Globals::thingDefaultPosList.end())
			{
				//Add it to the list
				leftPussyDefaultPos = leftPusObj->local.translate;
				Globals::thingDefaultPosList[leftpair] = leftPussyDefaultPos;
				LOG("Adding {} to default list for {} -> {} {} {}", Globals::leftPus.c_str(), actor->GetActorBase()->formID, leftPussyDefaultPos.x, leftPussyDefaultPos.y, leftPussyDefaultPos.z);

			}
			else
			{
				leftPussyDefaultPos = posMap->second;
			}

			auto rightpair = std::make_pair(actor->GetActorBase()->formID, Globals::rightPus.c_str());
			posMap = Globals::thingDefaultPosList.find(rightpair);

			if (posMap == Globals::thingDefaultPosList.end())
			{
				//Add it to the list
				rightPussyDefaultPos = rightPusObj->local.translate;
				Globals::thingDefaultPosList[rightpair] = rightPussyDefaultPos;
				LOG("Adding {} to default list for {} -> {} {} {}", Globals::rightPus.c_str(), actor->GetActorBase()->formID, rightPussyDefaultPos.x, rightPussyDefaultPos.y, rightPussyDefaultPos.z);

			}
			else
			{
				rightPussyDefaultPos = posMap->second;
			}

			auto backpair = std::make_pair(actor->GetActorBase()->formID, Globals::backPus.c_str());
			posMap = Globals::thingDefaultPosList.find(backpair);

			if (posMap == Globals::thingDefaultPosList.end())
			{
				//Add it to the list
				backPussyDefaultPos = backPusObj->local.translate;
				Globals::thingDefaultPosList[backpair] = backPussyDefaultPos;
				LOG("Adding {} to default list for {} -> {} {} {}", Globals::backPus.c_str(), actor->GetActorBase()->formID, backPussyDefaultPos.x, backPussyDefaultPos.y, backPussyDefaultPos.z);

			}
			else
			{
				backPussyDefaultPos = posMap->second;
			}

			auto frontpair = std::make_pair(actor->GetActorBase()->formID, Globals::frontPus.c_str());
			posMap = Globals::thingDefaultPosList.find(frontpair);

			if (posMap == Globals::thingDefaultPosList.end())
			{
				//Add it to the list
				frontPussyDefaultPos = frontPusObj->local.translate;
				Globals::thingDefaultPosList[frontpair] = frontPussyDefaultPos;
				LOG("Adding {} to default list for {} -> {} {} {}", Globals::frontPus.c_str(), actor->GetActorBase()->formID, frontPussyDefaultPos.x, frontPussyDefaultPos.y, frontPussyDefaultPos.z);

			}
			else
			{
				frontPussyDefaultPos = posMap->second;
			}
			thing_map_lock.unlock();
			LOG_INFO("Left pussy default pos -> {} {} {} , Right pussy default pos ->  {} {} {} , Back pussy default pos ->  {} {} {} , Front pussy default pos ->  {} {} {}", leftPussyDefaultPos.x, leftPussyDefaultPos.y, leftPussyDefaultPos.z, rightPussyDefaultPos.x, rightPussyDefaultPos.y, rightPussyDefaultPos.z, backPussyDefaultPos.x, backPussyDefaultPos.y, backPussyDefaultPos.z, frontPussyDefaultPos.x, frontPussyDefaultPos.y, frontPussyDefaultPos.z);

			CollisionConfig.CollisionMaxOffset = RE::NiPoint3(100, 100, 100);
			CollisionConfig.CollisionMinOffset = RE::NiPoint3(-100, -100, -100);
		}

		if (!ActorCollisionsEnabled)
		{
			return;
		}

		// Collision Stuff Start
		RE::NiPoint3 collisionVector = {};

		RE::NiMatrix3 pelvisRotation = pelvisObj->world.rotate;
		RE::NiPoint3 pelvisPosition = pelvisObj->world.translate;
		float pelvisScale = pelvisObj->world.scale;
		float pelvisInvScale = 1.0f / pelvisScale; //world transform pos to local transform pos edited by scale

		std::vector<int> thingIdList;
		std::vector<int> hashIdList;
		RE::NiPoint3 playerPos = RE::PlayerCharacter::GetSingleton()->loadedData->data3D->world.translate;
		float colliderNodescale = 1.0f - ((1.0f - (pelvisScale / actorBaseScale)) * scaleWeight);
		for (int i = 0; i < thingCollisionSpheres.size(); i++)
		{
			thingCollisionSpheres[i].offset100 = thingCollisionSpheres[i].offset0 * actorBaseScale * colliderNodescale;
			thingCollisionSpheres[i].worldPos = pelvisPosition + (pelvisRotation * thingCollisionSpheres[i].offset100);
			thingCollisionSpheres[i].radius100 = thingCollisionSpheres[i].radius0 * actorBaseScale * colliderNodescale;
			thingCollisionSpheres[i].radius100pwr2 = thingCollisionSpheres[i].radius100 * thingCollisionSpheres[i].radius100;
			hashIdList = GetHashIdsFromPos(thingCollisionSpheres[i].worldPos - playerPos, thingCollisionSpheres[i].radius100);
			for (int m = 0; m < hashIdList.size(); m++)
			{
				if (std::ranges::find(thingIdList, hashIdList[m]) == thingIdList.end())
				{
					thingIdList.emplace_back(hashIdList[m]);
				}
			}
		}
		for (int i = 0; i < thingCollisionCapsules.size(); i++)
		{
			thingCollisionCapsules[i].End1_offset100 = thingCollisionCapsules[i].End1_offset0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End1_worldPos = pelvisPosition + (pelvisRotation * thingCollisionCapsules[i].End1_offset100);
			thingCollisionCapsules[i].End1_radius100 = thingCollisionCapsules[i].End1_radius0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End1_radius100pwr2 = thingCollisionCapsules[i].End1_radius100 * thingCollisionCapsules[i].End1_radius100;
			thingCollisionCapsules[i].End2_offset100 = thingCollisionCapsules[i].End2_offset0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End2_worldPos = pelvisPosition + (pelvisRotation * thingCollisionCapsules[i].End2_offset100);
			thingCollisionCapsules[i].End2_radius100 = thingCollisionCapsules[i].End2_radius0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End2_radius100pwr2 = thingCollisionCapsules[i].End2_radius100 * thingCollisionCapsules[i].End2_radius100;
			hashIdList = GetHashIdsFromPos((thingCollisionCapsules[i].End1_worldPos + thingCollisionCapsules[i].End2_worldPos) * 0.5f - playerPos
				, (thingCollisionCapsules[i].End1_radius100 + thingCollisionCapsules[i].End2_radius100) * 0.5f);
			for (int m = 0; m < hashIdList.size(); m++)
			{
				if (std::ranges::find(thingIdList, hashIdList[m]) == thingIdList.end())
				{
					thingIdList.emplace_back(hashIdList[m]);
				}
			}
		}


		RE::NiPoint3 collisionDiff = {};

		CollisionConfig.maybePos = pelvisPosition;
		CollisionConfig.origRot = pelvisObj->parent->world.rotate;
		CollisionConfig.objRot = pelvisRotation;
		CollisionConfig.invRot = pelvisObj->parent->world.rotate.Transpose();
		bool genitalPenetration = false;
		for (int j = 0; j < thingIdList.size(); j++)
		{
			int id = thingIdList[j];
			if (Globals::partitions.find(id) != Globals::partitions.end())
			{
				LOG_INFO("Pelvis hashId={}", id);
				for (int i = 0; i < Globals::partitions[id].partitionCollisions.size(); i++)
				{
					if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && Globals::partitions[id].partitionCollisions[i].colliderNodeName.find("Genital") != std::string::npos)
						continue;

					if (IgnoreAllSelfColliders && Globals::partitions[id].partitionCollisions[i].colliderActor == actor)
						continue;

					if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && std::ranges::find(IgnoredSelfCollidersList, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != IgnoredSelfCollidersList.end())
						continue;

					if (std::ranges::find(IgnoredCollidersList, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != IgnoredCollidersList.end())
						continue;

					if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && std::strcmp(Globals::partitions[id].partitionCollisions[i].colliderNodeName.c_str(), boneName.c_str()) == 0)
						continue;

					if (Globals::debugtimelog || Globals::logging)
						InterlockedIncrement(&Globals::callCount);

					Globals::partitions[id].partitionCollisions[i].CollidedWeight = actorWeight;

					//now not that do reach max value just by get closer and just affected by the collider size
					bool isColliding = Globals::partitions[id].partitionCollisions[i].CheckPelvisCollision(collisionDiff, thingCollisionSpheres, thingCollisionCapsules, CollisionConfig, false);
					if (isColliding)
					{
						genitalPenetration = true;

						if (Globals::partitions[id].partitionCollisions[i].colliderActor == (RE::PlayerCharacter::GetSingleton()) && std::ranges::find(Globals::PlayerCollisionEventNodes, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != Globals::PlayerCollisionEventNodes.end())
						{
							Globals::ActorNodePlayerCollisionEventMap[actorNodeString].collisionInThisCycle = true;
						}
						if (Globals::partitions[id].partitionCollisions[i].colliderActor == (RE::PlayerCharacter::GetSingleton()) && Globals::partitions[id].partitionCollisions[i].colliderNodeName.find("Genital") != std::string::npos)
						{
							Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].collisionInThisCycle = true;
						}
					}
				}
			}
		}

		// Collision Stuff End
		RE::NiPoint3 leftVector = {};
		RE::NiPoint3 rightVector = {};
		RE::NiPoint3 backVector = {};
		RE::NiPoint3 frontVector = {};

		if (genitalPenetration)
		{
			//need to convert world pos diif to local pos diff due to node scale
			//but min/max limit follows node scale because it can cause like torn pussy in small actor
			float opening = distance(collisionDiff, {}) * pelvisInvScale;

			CalculateDiffVagina(leftVector, opening, true, true);
			CalculateDiffVagina(rightVector, opening, true, false);
			CalculateDiffVagina(backVector, opening, false, true);
			CalculateDiffVagina(frontVector, opening, false, false);

			NormalizeNiPoint(leftVector, thing_vaginaOpeningLimit * -1.0f, thing_vaginaOpeningLimit);
			NormalizeNiPoint(rightVector, thing_vaginaOpeningLimit * -1.0f, thing_vaginaOpeningLimit);
			backVector.y = clamp(backVector.y, thing_vaginaOpeningLimit * -0.5f, thing_vaginaOpeningLimit * 0.5f);
			backVector.z = clamp(backVector.z, thing_vaginaOpeningLimit * -0.165f, thing_vaginaOpeningLimit * 0.165f);
			frontVector.y = clamp(frontVector.y, thing_vaginaOpeningLimit * -0.125f, thing_vaginaOpeningLimit * 0.125f);
			frontVector.z = clamp(frontVector.z, thing_vaginaOpeningLimit * -0.25f, thing_vaginaOpeningLimit * 0.25f);
		}
		thing_SetNode_lock.lock();
		leftPusObj->local.translate = leftPussyDefaultPos + leftVector;
		rightPusObj->local.translate = rightPussyDefaultPos + rightVector;
		backPusObj->local.translate = backPussyDefaultPos + backVector;
		frontPusObj->local.translate = frontPussyDefaultPos + frontVector;

		RefreshNode(leftPusObj);
		RefreshNode(rightPusObj);
		RefreshNode(backPusObj);
		RefreshNode(frontPusObj);
		thing_SetNode_lock.unlock();
		/*QueryPerformanceCounter(&endingTime);
		elapsedMicroseconds.QuadPart = endingTime.QuadPart - startingTime.QuadPart;
		elapsedMicroseconds.QuadPart *= 1000000000LL;
		elapsedMicroseconds.QuadPart /= frequency.QuadPart;
		LOG("Thing.updatePelvis() Update Time = {} ns\n", elapsedMicroseconds.QuadPart);*/
	}

	void Thing::updateAnal(RE::Actor* actor, std::shared_mutex& thing_ReadNode_lock, std::shared_mutex& thing_SetNode_lock)
	{
		if (skipFramesPelvisCount > 0)
		{
			skipFramesPelvisCount--;
			return;
		}
		else
		{
			skipFramesPelvisCount = Globals::collisionSkipFramesPelvis;
		}

		if (!RE::PlayerCharacter::GetSingleton()->Is3DLoaded()) {
			return;
		}
	
		auto loadedState = actor->loadedData;
		if (!loadedState || !loadedState->data3D)
		{
			return;
		}
		thing_ReadNode_lock.lock();
		RE::NiAVObject* analObj = actor->loadedData->data3D->GetObjectByName(Globals::anal);
		RE::NiAVObject* leftAnusObj = actor->loadedData->data3D->GetObjectByName(Globals::leftAnus);
		RE::NiAVObject* rightAnusObj = actor->loadedData->data3D->GetObjectByName(Globals::rightAnus);
		RE::NiAVObject* frontAnusObj = actor->loadedData->data3D->GetObjectByName(Globals::frontAnus);
		RE::NiAVObject* backAnusObj = actor->loadedData->data3D->GetObjectByName(Globals::backAnus);
		thing_ReadNode_lock.unlock();
		if (!analObj || !leftAnusObj || !rightAnusObj || !frontAnusObj || !backAnusObj)
			return;
		if (updateAnalFirstRun)
		{
			updateAnalFirstRun = false;
			auto leftpair = std::make_pair(actor->GetActorBase()->formID, Globals::leftAnus);
			thing_map_lock.lock();
			auto posMap = Globals::thingDefaultPosList.find(leftpair);
			if (posMap == Globals::thingDefaultPosList.end())
			{
				//Add it to the list
				//leftAnusDefaultPos = leftAnusObj->local.translate;
				leftAnusDefaultPos = RE::NiPoint3(-0.753204f, -0.059860f, 0.431540f); //rig file of XPMSSE 4.6 or higher version has often anus bone torsion, so I hardcoded bone location
				Globals::thingDefaultPosList[leftpair] = leftAnusDefaultPos;
				LOG("Adding {} to default list for {} -> {} {} {}", Globals::leftAnus.c_str(), actor->GetActorBase()->formID, leftAnusDefaultPos.x, leftAnusDefaultPos.y, leftAnusDefaultPos.z);
			}
			else
			{
				leftAnusDefaultPos = posMap->second;
			}
			auto rightpair = std::make_pair(actor->GetActorBase()->formID, Globals::rightAnus);
			posMap = Globals::thingDefaultPosList.find(rightpair);
			if (posMap == Globals::thingDefaultPosList.end())
			{
				//Add it to the list
				//rightAnusDefaultPos = rightAnusObj->local.translate;
				rightAnusDefaultPos = RE::NiPoint3(-0.753204f, 0.059856f, 0.431471f); //rig file of XPMSSE 4.6 or higher version has often anus bone torsion, so I hardcoded bone location
				Globals::thingDefaultPosList[rightpair] = rightAnusDefaultPos;
				LOG("Adding {} to default list for {} -> {} {} {}", Globals::rightAnus.c_str(), actor->GetActorBase()->formID, rightAnusDefaultPos.x, rightAnusDefaultPos.y, rightAnusDefaultPos.z);
			}
			else
			{
				rightAnusDefaultPos = posMap->second;
			}
			auto backpair = std::make_pair(actor->GetActorBase()->formID, Globals::backAnus);
			posMap = Globals::thingDefaultPosList.find(backpair);
			if (posMap == Globals::thingDefaultPosList.end())
			{
				//Add it to the list
				//backAnusDefaultPos = backAnusObj->local.translate;
				backAnusDefaultPos = RE::NiPoint3(-0.753212f, 0.048635f, 0.432867f); //rig file of XPMSSE 4.6 or higher version has often anus bone torsion, so I hardcoded bone location
				Globals::thingDefaultPosList[backpair] = backAnusDefaultPos;
				LOG("Adding {} to default list for {} -> {} {} {}", Globals::backAnus.c_str(), actor->GetActorBase()->formID, backAnusDefaultPos.x, backAnusDefaultPos.y, backAnusDefaultPos.z);
			}
			else
			{
				backAnusDefaultPos = posMap->second;
			}
			auto frontpair = std::make_pair(actor->GetActorBase()->formID, Globals::frontAnus);
			posMap = Globals::thingDefaultPosList.find(frontpair);
			if (posMap == Globals::thingDefaultPosList.end())
			{
				//Add it to the list
				//frontAnusDefaultPos = frontAnusObj->local.translate;
				frontAnusDefaultPos = RE::NiPoint3(-0.753235f, -0.048630f, 0.432873f); //rig file of XPMSSE 4.6 or higher version has often anus bone torsion, so I hardcoded bone location
				Globals::thingDefaultPosList[frontpair] = frontAnusDefaultPos;
				LOG("Adding {} to default list for {} -> {} {} {}", Globals::frontAnus.c_str(), actor->GetActorBase()->formID, frontAnusDefaultPos.x, frontAnusDefaultPos.y, frontAnusDefaultPos.z);
			}
			else
			{
				frontAnusDefaultPos = posMap->second;
			}
			thing_map_lock.unlock();
			LOG_INFO("Left anus default pos -> {} {} {} , Right anus default pos ->  {} {} {} , Back anus default pos ->  {} {} {} , Front anus default pos ->  {} {} {}", leftAnusDefaultPos.x, leftAnusDefaultPos.y, leftAnusDefaultPos.z, rightAnusDefaultPos.x, rightAnusDefaultPos.y, rightAnusDefaultPos.z, backAnusDefaultPos.x, backAnusDefaultPos.y, backAnusDefaultPos.z, frontAnusDefaultPos.x, frontAnusDefaultPos.y, frontAnusDefaultPos.z);
			CollisionConfig.CollisionMaxOffset = RE::NiPoint3(100, 100, 100);
			CollisionConfig.CollisionMinOffset = RE::NiPoint3(-100, -100, -100);
		}
		if (!ActorCollisionsEnabled)
		{
			return;
		}
		RE::NiMatrix3 analnodeRotation = analObj->world.rotate;
		RE::NiPoint3 analnodePosition = analObj->world.translate;
		float analnodeScale = analObj->world.scale;
		float analnodeInvScale = 1.0f / analObj->parent->world.scale; //world transform pos to local transform pos edited by scale
		std::vector<int> thingIdList;
		std::vector<int> hashIdList;
		RE::NiPoint3 playerPos = RE::PlayerCharacter::GetSingleton()->loadedData->data3D->world.translate;
		float colliderNodescale = 1.0f - ((1.0f - (analnodeScale / actorBaseScale)) * scaleWeight);
		for (int i = 0; i < thingCollisionSpheres.size(); i++)
		{
			thingCollisionSpheres[i].offset100 = thingCollisionSpheres[i].offset0 * actorBaseScale * colliderNodescale;
			thingCollisionSpheres[i].worldPos = analnodePosition + (analnodeRotation * thingCollisionSpheres[i].offset100);
			thingCollisionSpheres[i].radius100 = thingCollisionSpheres[i].radius0 * actorBaseScale * colliderNodescale;
			thingCollisionSpheres[i].radius100pwr2 = thingCollisionSpheres[i].radius100 * thingCollisionSpheres[i].radius100;
			hashIdList = GetHashIdsFromPos(thingCollisionSpheres[i].worldPos - playerPos, thingCollisionSpheres[i].radius100);
			for (int m = 0; m < hashIdList.size(); m++)
			{
				if (std::ranges::find(thingIdList, hashIdList[m]) == thingIdList.end())
				{
					thingIdList.emplace_back(hashIdList[m]);
				}
			}
		}
		for (int i = 0; i < thingCollisionCapsules.size(); i++)
		{
			thingCollisionCapsules[i].End1_offset100 = thingCollisionCapsules[i].End1_offset0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End1_worldPos = analnodePosition + (analnodeRotation * thingCollisionCapsules[i].End1_offset100);
			thingCollisionCapsules[i].End1_radius100 = thingCollisionCapsules[i].End1_radius0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End1_radius100pwr2 = thingCollisionCapsules[i].End1_radius100 * thingCollisionCapsules[i].End1_radius100;
			thingCollisionCapsules[i].End2_offset100 = thingCollisionCapsules[i].End2_offset0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End2_worldPos = analnodePosition + (analnodeRotation * thingCollisionCapsules[i].End2_offset100);
			thingCollisionCapsules[i].End2_radius100 = thingCollisionCapsules[i].End2_radius0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End2_radius100pwr2 = thingCollisionCapsules[i].End2_radius100 * thingCollisionCapsules[i].End2_radius100;
			hashIdList = GetHashIdsFromPos((thingCollisionCapsules[i].End1_worldPos + thingCollisionCapsules[i].End2_worldPos) * 0.5f - playerPos
				, (thingCollisionCapsules[i].End1_radius100 + thingCollisionCapsules[i].End2_radius100) * 0.5f);
			for (int m = 0; m < hashIdList.size(); m++)
			{
				if (std::ranges::find(thingIdList, hashIdList[m]) == thingIdList.end())
				{
					thingIdList.emplace_back(hashIdList[m]);
				}
			}
		}
		RE::NiPoint3 collisionDiff = {};
		CollisionConfig.maybePos = analnodePosition;
		CollisionConfig.origRot = analObj->parent->world.rotate;
		CollisionConfig.objRot = analnodeRotation;
		CollisionConfig.invRot = analObj->parent->world.rotate.Transpose();
		bool genitalPenetration = false;
		for (int j = 0; j < thingIdList.size(); j++)
		{
			int id = thingIdList[j];
			if (Globals::partitions.find(id) != Globals::partitions.end())
			{
				for (int i = 0; i < Globals::partitions[id].partitionCollisions.size(); i++)
				{
					if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && Globals::partitions[id].partitionCollisions[i].colliderNodeName.find("Genital") != std::string::npos)
						continue;

					if (IgnoreAllSelfColliders && Globals::partitions[id].partitionCollisions[i].colliderActor == actor)
						continue;

					if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && std::ranges::find(IgnoredSelfCollidersList, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != IgnoredSelfCollidersList.end())
						continue;

					if (std::ranges::find(IgnoredCollidersList, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != IgnoredCollidersList.end())
						continue;

					if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && std::strcmp(Globals::partitions[id].partitionCollisions[i].colliderNodeName.c_str(), boneName.c_str()) == 0)
						continue;

					if (Globals::debugtimelog || Globals::logging)
						InterlockedIncrement(&Globals::callCount);

					Globals::partitions[id].partitionCollisions[i].CollidedWeight = actorWeight;

					bool isColliding = Globals::partitions[id].partitionCollisions[i].CheckPelvisCollision(collisionDiff, thingCollisionSpheres, thingCollisionCapsules, CollisionConfig, false);
					if (isColliding)
					{
						genitalPenetration = true;

						if (Globals::partitions[id].partitionCollisions[i].colliderActor == (RE::PlayerCharacter::GetSingleton()) && std::ranges::find(Globals::PlayerCollisionEventNodes, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != Globals::PlayerCollisionEventNodes.end())
						{
							Globals::ActorNodePlayerCollisionEventMap[actorNodeString].collisionInThisCycle = true;
						}
						if (Globals::partitions[id].partitionCollisions[i].colliderActor == (RE::PlayerCharacter::GetSingleton()) && Globals::partitions[id].partitionCollisions[i].colliderNodeName.find("Genital") != std::string::npos)
						{
							Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].collisionInThisCycle = true;
						}
					}
				}
			}
		}
		// Collision Stuff End
		RE::NiPoint3 leftVector = {};
		RE::NiPoint3 rightVector = {};
		RE::NiPoint3 backVector = {};
		RE::NiPoint3 frontVector = {};
		if (genitalPenetration)
		{
			float opening = distance(collisionDiff, {}) * analnodeScale;
			CalculateDiffAnus(leftVector, opening, true, true);
			CalculateDiffAnus(rightVector, opening, true, false);
			CalculateDiffAnus(backVector, opening, false, true);
			CalculateDiffAnus(frontVector, opening, false, false);
			NormalizeNiPoint(leftVector, thing_anusOpeningLimit * -1.0f, thing_anusOpeningLimit);
			NormalizeNiPoint(rightVector, thing_anusOpeningLimit * -1.0f, thing_anusOpeningLimit);
			backVector.x = clamp(backVector.x, thing_anusOpeningLimit * -0.8f, thing_anusOpeningLimit * 0.8f);
			backVector.z = clamp(backVector.z, thing_anusOpeningLimit * -0.32f, thing_anusOpeningLimit * 0.32f);
			frontVector.x = clamp(frontVector.x, thing_anusOpeningLimit * -0.8f, thing_anusOpeningLimit * 0.8f);
			frontVector.z = clamp(frontVector.z, thing_anusOpeningLimit * -0.32f, thing_anusOpeningLimit * 0.32f);
		}
		thing_SetNode_lock.lock();
		leftAnusObj->local.translate = leftAnusDefaultPos + leftVector;
		rightAnusObj->local.translate = rightAnusDefaultPos + rightVector;
		backAnusObj->local.translate = backAnusDefaultPos + backVector;
		frontAnusObj->local.translate = frontAnusDefaultPos + frontVector;
		RefreshNode(leftAnusObj);
		RefreshNode(rightAnusObj);
		RefreshNode(backAnusObj);
		RefreshNode(frontAnusObj);
		thing_SetNode_lock.unlock();
	}

	bool Thing::ApplyBellyBulge(RE::Actor* actor, std::shared_mutex& thing_ReadNode_lock, std::shared_mutex& thing_SetNode_lock)
	{
		if (!RE::PlayerCharacter::GetSingleton()->Is3DLoaded()){
			return false;
		}

		RE::NiPoint3 collisionVector = {};

		thing_ReadNode_lock.lock();
		RE::NiAVObject* bulgeObj = actor->loadedData->data3D->GetObjectByName(Globals::spine1);
		RE::NiAVObject* bellyObj = actor->loadedData->data3D->GetObjectByName(Globals::belly);
		thing_ReadNode_lock.unlock();

		if (!bellyObj || !bulgeObj)
			return false;

		if (updateBellyFirstRun)
		{
			updateBellyFirstRun = false;

			auto mypair = std::make_pair(actor->GetActorBase()->formID, Globals::belly);
			thing_map_lock.lock();
			auto posMap = Globals::thingDefaultPosList.find(mypair);

			if (posMap == Globals::thingDefaultPosList.end())
			{
				//Add it to the list
				bellyDefaultPos = {};
				Globals::thingDefaultPosList[mypair] = bellyDefaultPos;
				LOG("Adding {} to default list for {} -> {} {} {}", Globals::belly.c_str(), actor->GetActorBase()->formID, bellyDefaultPos.x, bellyDefaultPos.y, bellyDefaultPos.z);
			}
			else
			{
				bellyDefaultPos = {};
			}
			thing_map_lock.unlock();
			LOG_INFO("Belly default pos -> {} {} {}", bellyDefaultPos.x, bellyDefaultPos.y, bellyDefaultPos.z);
			CollisionConfig.CollisionMaxOffset = RE::NiPoint3(100, 100, 100);
			CollisionConfig.CollisionMinOffset = RE::NiPoint3(-100, -100, -100);
		}

		RE::NiMatrix3 bulgenodeRotation = bulgeObj->world.rotate;
		RE::NiPoint3 bulgenodePosition = bulgeObj->world.translate;
		float bulgenodeScale = bulgeObj->world.scale;
		float bellynodeInvScale = 1.0f / bellyObj->parent->world.scale; //world transform pos to local transform pos edited by scale

		std::vector<int> thingIdList;
		std::vector<int> hashIdList;

		RE::NiPoint3 playerPos = RE::PlayerCharacter::GetSingleton()->loadedData->data3D->world.translate;

		float colliderNodescale = 1.0f - ((1.0f - (bulgenodeScale / actorBaseScale)) * scaleWeight);
		for (int i = 0; i < thingCollisionSpheres.size(); i++)
		{
			thingCollisionSpheres[i].offset100 = thingCollisionSpheres[i].offset0 * actorBaseScale * colliderNodescale;
			thingCollisionSpheres[i].worldPos = bulgenodePosition + (bulgenodeRotation * thingCollisionSpheres[i].offset100);
			thingCollisionSpheres[i].radius100 = thingCollisionSpheres[i].radius0 * actorBaseScale * colliderNodescale;
			thingCollisionSpheres[i].radius100pwr2 = thingCollisionSpheres[i].radius100 * thingCollisionSpheres[i].radius100;
			hashIdList = GetHashIdsFromPos(thingCollisionSpheres[i].worldPos - playerPos, thingCollisionSpheres[i].radius100);
			for (int m = 0; m < hashIdList.size(); m++)
			{
				if (std::ranges::find(thingIdList, hashIdList[m]) == thingIdList.end())
				{
					thingIdList.emplace_back(hashIdList[m]);
				}
			}
		}
		for (int i = 0; i < thingCollisionCapsules.size(); i++)
		{
			thingCollisionCapsules[i].End1_offset100 = thingCollisionCapsules[i].End1_offset0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End1_worldPos = bulgenodePosition + (bulgenodeRotation * thingCollisionCapsules[i].End1_offset100);
			thingCollisionCapsules[i].End1_radius100 = thingCollisionCapsules[i].End1_radius0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End1_radius100pwr2 = thingCollisionCapsules[i].End1_radius100 * thingCollisionCapsules[i].End1_radius100;
			thingCollisionCapsules[i].End2_offset100 = thingCollisionCapsules[i].End2_offset0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End2_worldPos = bulgenodePosition + (bulgenodeRotation * thingCollisionCapsules[i].End2_offset100);
			thingCollisionCapsules[i].End2_radius100 = thingCollisionCapsules[i].End2_radius0 * actorBaseScale * colliderNodescale;
			thingCollisionCapsules[i].End2_radius100pwr2 = thingCollisionCapsules[i].End2_radius100 * thingCollisionCapsules[i].End2_radius100;
			hashIdList = GetHashIdsFromPos((thingCollisionCapsules[i].End1_worldPos + thingCollisionCapsules[i].End2_worldPos) * 0.5f - playerPos
				, (thingCollisionCapsules[i].End1_radius100 + thingCollisionCapsules[i].End2_radius100) * 0.5f);
			for (int m = 0; m < hashIdList.size(); m++)
			{
				if (std::ranges::find(thingIdList, hashIdList[m]) == thingIdList.end())
				{
					thingIdList.emplace_back(hashIdList[m]);
				}
			}
		}

		RE::NiPoint3 collisionDiff = {};

		CollisionConfig.maybePos = bulgenodePosition;
		CollisionConfig.origRot = bulgeObj->parent->world.rotate;
		CollisionConfig.objRot = bulgenodeRotation;
		CollisionConfig.invRot = bulgeObj->parent->world.rotate.Transpose();
		bool genitalPenetration = false;

		for (int j = 0; j < thingIdList.size(); j++)
		{
			int id = thingIdList[j];
			if (Globals::partitions.find(id) != Globals::partitions.end())
			{
				for (int i = 0; i < Globals::partitions[id].partitionCollisions.size(); i++)
				{
					if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && Globals::partitions[id].partitionCollisions[i].colliderNodeName.find("Genital") != std::string::npos)
						continue;

					if (IgnoreAllSelfColliders && Globals::partitions[id].partitionCollisions[i].colliderActor == actor)
						continue;

					if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && std::ranges::find(IgnoredSelfCollidersList, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != IgnoredSelfCollidersList.end())
						continue;

					if (std::ranges::find(IgnoredCollidersList, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != IgnoredCollidersList.end())
						continue;

					if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && std::strcmp(Globals::partitions[id].partitionCollisions[i].colliderNodeName.c_str(), boneName.c_str()) == 0)
						continue;

					if (thing_bellybulgelist.size() > 0) // If the list is empty, assume everything goes
					{
						if (std::ranges::find(thing_bellybulgelist, Globals::partitions[id].partitionCollisions[i].colliderNodeName) == thing_bellybulgelist.end())
							continue;
					}

					if (thing_bellybulgeblacklist.size() > 0)
					{
						if (std::ranges::find(thing_bellybulgeblacklist, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != thing_bellybulgeblacklist.end())
							continue;
					}

					if (Globals::debugtimelog || Globals::logging)
						InterlockedIncrement(&Globals::callCount);

					Globals::partitions[id].partitionCollisions[i].CollidedWeight = actorWeight;

					//now not that do reach max value just by get closer and just affected by the collider size
					bool isColliding = Globals::partitions[id].partitionCollisions[i].CheckPelvisCollision(collisionDiff, thingCollisionSpheres, thingCollisionCapsules, CollisionConfig, true);

					if (isColliding)
					{
						genitalPenetration = true;
					}
				}
			}
		}

		if (genitalPenetration)
		{
			newBellyBulge = distance(collisionDiff, {});

			return true;
		}

		newBellyBulge = 0.0f;

		return false;
	}

	void Thing::update(RE::Actor* actor, std::shared_mutex& thing_ReadNode_lock, std::shared_mutex& thing_SetNode_lock) {

		bool collisionsOn = true;
		float Friction = 1.0f;
		if (collisionOnLastFrame)
		{
			Friction = collisionFriction;
		}
		if (skipFramesCount > 0)
		{
			skipFramesCount--;
			collisionsOn = false;
			if (collisionOnLastFrame)
			{
				return;
			}
		}
		else
		{
			skipFramesCount = Globals::collisionSkipFrames;
			collisionOnLastFrame = false;
		}

		auto newTime = clock();
		bool isSkippedmanyFrames = false;
		if (newTime - time >= 200) // Frame blank for more than 0.2 sec / 5 fps
			isSkippedmanyFrames = true;
		time = newTime;

		long deltaT = Globals::IntervalTimeTick * 1000;
		float fpsCorrection = 1.0f;
		if (Globals::fpsCorrectionEnabled)
		{
			if (deltaT > 50)
			{
				deltaT = 50; //fps min 20 //To prevent infinite shaking at very low fps, it's seems better to calibrate to a minimum of 20 fps
				fpsCorrection = 3.0f;
			}
			else if (deltaT < 1)
			{
				deltaT = 1; //fps max 1000
				fpsCorrection = 0.06f;
			}
			else
				fpsCorrection = Globals::IntervalTimeTickScale; // = deltaT / (1sec / (fps60tick = 1 / 60 * 1000 = 16))
		}
		else
		{
			if (deltaT > 64) deltaT = 64;
			if (deltaT < 8) deltaT = 8;
		}

		if (!RE::PlayerCharacter::GetSingleton()->Is3DLoaded())
		{
			return;
		}

		RE::NiAVObject* obj;
		auto loadedState = actor->loadedData;

		if (!loadedState || !loadedState->data3D) {
			LOG("No loaded state for actor {}\n", actor->formID);
			return;
		}
		thing_ReadNode_lock.lock();
		obj = loadedState->data3D->GetObjectByName(boneName);
		thing_ReadNode_lock.unlock();

		if (!obj || !obj->parent)
			return;

		float nodeScale = obj->world.scale;
		float nodeParentInvScale = 1.0f / obj->parent->world.scale; //need to convert world transform pos to local transform pos due to node scale

		RE::NiPoint3 newPos = oldWorldPos;
		RE::NiPoint3 newPosRot = oldWorldPosRot;

		if (isSkippedmanyFrames) //prevents many bounce when fps gaps
		{
			oldWorldPos = obj->parent->world.translate + obj->parent->world.rotate * oldLocalPos;
			oldWorldPosRot = obj->parent->world.translate + obj->parent->world.rotate * oldLocalPosRot;
			return;
		}

		bool IsThereCollision = false;
		bool maybeNot = false;
		RE::NiPoint3 collisionDiff = {};
		long originalDeltaT = deltaT;
		long deltaTRot = deltaT;
		RE::NiPoint3 collisionVector = {};

		float varGravityBias = gravityBias;

		float varLinearX = linearX;
		float varLinearY = linearY;
		float varLinearZ = linearZ;
		float varRotationalXnew = rotationalXnew;
		float varRotationalYnew = rotationalYnew;
		float varRotationalZnew = rotationalZnew;


		int collisionCheckCount = 0;

		RE::NiPoint3 posDelta = {};
		RE::NiPoint3 posDeltaRot = {};

		RE::NiPoint3 target = obj->parent->world.translate;

		if (IsBreastBone) //other bones don't need to edited gravity by NPC Spine2 [Spn2] obj
		{
			//Get the reference bone to know which way the breasts are orientated
			thing_ReadNode_lock.lock();
			RE::NiAVObject* breastGravityReferenceBone = loadedState->data3D->GetObjectByName(Globals::breastGravityReferenceBoneString.c_str());
			thing_ReadNode_lock.unlock();

			float gravityRatio = 1.0f;
			//Code sent by KheiraDjet(modified)
			if (breastGravityReferenceBone != nullptr)
			{
				//Get the orientation (here the Z element of the rotation matrix (1.0 when standing up, -1.0 when upside down))			
				gravityRatio = (breastGravityReferenceBone->world.rotate.entry[2][2] + 1.0f) * 0.5f;
			}
			else
			{
				gravityRatio = (obj->parent->world.rotate.entry[2][2] + 1.0f) * 0.5f;
			}

			//Remap the value from 0.0 => 1.0 to user defined values and clamps it
			gravityRatio = remap(gravityRatio, gravityInvertedCorrectionStart, gravityInvertedCorrectionEnd, 0.0f, 1.0f);
			gravityRatio = clamp(gravityRatio, 0.0f, 1.0f);

			//Calculate the resulting gravity
			varGravityCorrection = (gravityRatio * gravityCorrection) + ((1.0f - gravityRatio) * gravityInvertedCorrection);

			//Determine which armor the actor is wearing
			if (skipArmorCheck <= 0) //This is a little heavy, check only on equip/unequip events
			{
				forceAmplitude = 1.0f;

				//thing_armorKeyword_lock.lock();
				RE::TESObjectARMO* armor = actor->GetWornArmor(0x00000004);

				if (true)
				{
					if (armor != nullptr)
					{
						bool IsAsNaked = false;
						isHeavyArmor = false;
						isLightArmor = false;
						isClothed = false;
						isNoPushUp = false;
						RE::BGSKeyword** keywords = armor->keywords;
						if (keywords)
						{
							if (IsLeftBreastBone)
							{
								for (uint32_t index = 0; index < armor->numKeywords; index++)
								{
									if (!keywords[index])
										continue;
									if (strcmp(keywords[index]->GetName(), Globals::KeywordNameAsNakedL.c_str()) == 0)
									{
										IsAsNaked = true;
									}
									else if (strcmp(keywords[index]->GetName(), Globals::KeywordNameAsHeavyL.c_str()) == 0)
									{
										isHeavyArmor = true;
									}
									else if (strcmp(keywords[index]->GetName(), Globals::KeywordNameAsLightL.c_str()) == 0)
									{
										isLightArmor = true;
									}
									else if (strcmp(keywords[index]->GetName(), Globals::KeywordNameAsClothingL.c_str()) == 0)
									{
										isClothed = true;
									}
									else if (strcmp(keywords[index]->GetName(), Globals::KeywordNameNoPushUpL.c_str()) == 0)
									{
										isNoPushUp = true;
									}

								}
							}
							else if (IsRightBreastBone)
							{
								for (uint32_t index = 0; index < armor->numKeywords; index++)
								{
									if (!keywords[index])
										continue;
									if (strcmp(keywords[index]->GetName(), Globals::KeywordNameAsNakedR.c_str()) == 0)
									{
										IsAsNaked = true;
									}
									else if (strcmp(keywords[index]->GetName(), Globals::KeywordNameAsHeavyR.c_str()) == 0)
									{
										isHeavyArmor = true;
									}
									else if (strcmp(keywords[index]->GetName(), Globals::KeywordNameAsLightR.c_str()) == 0)
									{
										isLightArmor = true;
									}
									else if (strcmp(keywords[index]->GetName(), Globals::KeywordNameAsClothingR.c_str()) == 0)
									{
										isClothed = true;
									}
									else if (strcmp(keywords[index]->GetName(), Globals::KeywordNameNoPushUpR.c_str()) == 0)
									{
										isNoPushUp = true;
									}
								}
							}
						}

						if (IsAsNaked)
						{
							isHeavyArmor = false;
							isLightArmor = false;
							isClothed = false;
						}
						else if (!isHeavyArmor && !isLightArmor && !isClothed)
						{
							isHeavyArmor = (armor->HasKeyword(Globals::KeywordArmorHeavy));
							isLightArmor = (armor->HasKeyword(Globals::KeywordArmorLight));
							isClothed = (armor->HasKeyword(Globals::KeywordArmorClothing));

						}
					}
					else
					{
						isClothed = false;
						isLightArmor = false;
						isHeavyArmor = false;
					}
				}
				else
				{
					isClothed = false;
					isLightArmor = false;
					isHeavyArmor = false;
				}
				skipArmorCheck--;

				if (isHeavyArmor)
				{
					if (!isNoPushUp)
						varGravityCorrection = varGravityCorrection + breastHeavyArmoredPushup;
					forceAmplitude = breastHeavyArmoredAmplitude;
				}
				else if (isLightArmor)
				{
					if (!isNoPushUp)
						varGravityCorrection = varGravityCorrection + breastLightArmoredPushup;
					forceAmplitude = breastLightArmoredAmplitude;
				}
				else if (isClothed)
				{
					if (!isNoPushUp)
						varGravityCorrection = varGravityCorrection + breastClothedPushup;
					forceAmplitude = breastClothedAmplitude;
				}
			}
		}
		else //other nodes are based on parent obj
		{
			//Get the orientation (here the Z element of the rotation matrix (1.0 when standing up, -1.0 when upside down))			
			float gravityRatio = (obj->parent->world.rotate.entry[2][2] + 1.0f) * 0.5f;

			//Remap the value from 0.0 => 1.0 to user defined values and clamps it
			gravityRatio = remap(gravityRatio, gravityInvertedCorrectionStart, gravityInvertedCorrectionEnd, 0.0f, 1.0f);
			gravityRatio = clamp(gravityRatio, 0.0f, 1.0f);

			//Calculate the resulting gravity
			varGravityCorrection = (gravityRatio * gravityCorrection) + ((1.0f - gravityRatio) * gravityInvertedCorrection);
		}

		//Offset to move Center of Mass make rotaional motion more significant  
		RE::NiPoint3 diff = (target - oldWorldPos) * forceMultipler;
		RE::NiPoint3 diffRot = (target - oldWorldPosRot) * forceMultipler;

		//It is not recommended to use, When used, there is a high possibility that the movement will be adversely affected due to min/maxoffset
		//diff += NiPoint3(0, 0, varGravityCorrection) * (fpsCorrectionEnabled ? fpsCorrection : 1.0f);

		if (fabs(diff.x) > 250.0f || fabs(diff.y) > 250.0f || fabs(diff.z) > 250.0f) //prevent shakes
		{
			//logger.error("transform reset\n");
			thing_SetNode_lock.lock();
			obj->local.translate = thingDefaultPos;
			obj->local.rotate = thingDefaultRot;
			thing_SetNode_lock.unlock();

			oldWorldPos = obj->parent->world.translate;
			oldWorldPosRot = obj->parent->world.translate;
			velocity = {};
			velocityRot = {};
			time = clock();
			return;
		}

		// move the bones based on the supplied weightings
		// Convert the world translations into local coordinates
		auto invRot = obj->parent->world.rotate.Transpose();
		RE::NiPoint3 forceGravityBias = invRot * (RE::NiPoint3(0.0f, 0.0f, varGravityBias) / fpsCorrection);
		RE::NiPoint3 lvarGravityCorrection = (invRot * RE::NiPoint3(0.0f, 0.0f, varGravityCorrection));

		RE::NiPoint3 ldiff = {};
		RE::NiPoint3 ldiffRot = {};
		RE::NiMatrix3 newRot;
		tbb::parallel_invoke(
			[&] { //linear calculation
				RE::NiPoint3 InteriaMaxOffset = {};
				RE::NiPoint3 InteriaMinOffset = {};
				// when collisionElastic is 1 and collided, remove jitter caused by Max/Minoffsets
				if (multiplerInertia >= 0.001f)
				{
					if (collisionInertia.x > XmaxOffset)
						InteriaMaxOffset.x = collisionInertia.x - XmaxOffset;
					else if (collisionInertia.x < XminOffset)
						InteriaMinOffset.x = collisionInertia.x - XminOffset;
					if (collisionInertia.y > YmaxOffset)
						InteriaMaxOffset.y = collisionInertia.y - YmaxOffset;
					else if (collisionInertia.y < YminOffset)
						InteriaMinOffset.y = collisionInertia.y - YminOffset;
					if (collisionInertia.z > ZmaxOffset)
						InteriaMaxOffset.z = collisionInertia.z - ZmaxOffset;
					else if (collisionInertia.z < ZminOffset)
						InteriaMinOffset.z = collisionInertia.z - ZminOffset;
					multiplerInertia -= ((static_cast<float>(originalDeltaT) / timeTick) * 0.01f * timeStep);
					if (multiplerInertia < 0.001f)
						multiplerInertia = 0.0f;
					InteriaMaxOffset *= multiplerInertia;
					InteriaMinOffset *= multiplerInertia;
				}

				// Compute the "Spring" Force
				float timeMultiplier = timeTick / static_cast<float>(deltaT);
				diff = invRot * (diff * timeMultiplier);
				RE::NiPoint3 stiffnessXYZ(stiffnessX, stiffnessY, stiffnessZ);
				RE::NiPoint3 stiffness2XYZ(stiffness2X, stiffness2Y, stiffness2Z);
				RE::NiPoint3 dampingXYZ(dampingX, dampingY, dampingZ);
				RE::NiPoint3 diff2(diff.x * diff.x * sgn(diff.x), diff.y * diff.y * sgn(diff.y), diff.z * diff.z * sgn(diff.z));
				RE::NiPoint3 force = (RE::NiPoint3((diff.x * stiffnessXYZ.x) + (diff2.x * stiffness2XYZ.x)
				                                   , (diff.y * stiffnessXYZ.y) + (diff2.y * stiffness2XYZ.y)
				                                   , (diff.z * stiffnessXYZ.z) + (diff2.z * stiffness2XYZ.z))
					- forceGravityBias);

				//showPos(diff);
				//showPos(force);

				velocity = invRot * velocity;
				do {
					// Assume mass is 1, so Accelleration is Force, can vary mass by changinf force
					//velocity = (velocity + (force * timeStep)) * (1 - (damping * timeStep));
					velocity = velocity * Friction; //Fixes that when becomes unstable collisions during colliding at low or unstable FPS
					velocity.x = (velocity.x + (force.x * timeStep)) - (velocity.x * (dampingXYZ.x * timeStep));
					velocity.y = (velocity.y + (force.y * timeStep)) - (velocity.y * (dampingXYZ.y * timeStep));
					velocity.z = (velocity.z + (force.z * timeStep)) - (velocity.z * (dampingXYZ.z * timeStep));

					posDelta += velocity * timeStep;

					deltaT -= timeTick;
				} while (deltaT >= timeTick);

				velocity = obj->parent->world.rotate * velocity; //velocity is maintain on world transform

				newPos = newPos + obj->parent->world.rotate * (posDelta * fpsCorrection);

				// clamp the difference to stop the breast severely lagging at low framerates
				RE::NiPoint3 newdiff = newPos - target;

				varLinearX = varLinearX * forceAmplitude * amplitude;
				varLinearY = varLinearY * forceAmplitude * amplitude;
				varLinearZ = varLinearZ * forceAmplitude * amplitude;


				ldiff = invRot * newdiff;
				auto beforeldiff = ldiff;

				ldiff.x = clamp(ldiff.x, XminOffset + InteriaMinOffset.x, XmaxOffset + InteriaMaxOffset.x);
				ldiff.y = clamp(ldiff.y, YminOffset + InteriaMinOffset.y, YmaxOffset + InteriaMaxOffset.y);
				ldiff.z = clamp(ldiff.z, ZminOffset + InteriaMinOffset.z, ZmaxOffset + InteriaMaxOffset.z);

				//It can allows the force of dissipated by min/maxoffsets to be spread in different directions
				beforeldiff = beforeldiff - ldiff;
				ldiff.x = ldiff.x + ((beforeldiff.y * linearYspreadforceX) + (beforeldiff.z * linearZspreadforceX));
				ldiff.y = ldiff.y + ((beforeldiff.x * linearXspreadforceY) + (beforeldiff.z * linearZspreadforceY));
				ldiff.z = ldiff.z + ((beforeldiff.x * linearXspreadforceZ) + (beforeldiff.y * linearYspreadforceZ));

				ldiff.x = clamp(ldiff.x, XminOffset + InteriaMinOffset.x, XmaxOffset + InteriaMaxOffset.x);
				ldiff.y = clamp(ldiff.y, YminOffset + InteriaMinOffset.y, YmaxOffset + InteriaMaxOffset.y);
				ldiff.z = clamp(ldiff.z, ZminOffset + InteriaMinOffset.z, ZmaxOffset + InteriaMaxOffset.z);
				//same the clamp(diff.z - varGravityCorrection, -maxOffset, maxOffset) + varGravityCorrection
				//this is the reason for the endless shaking when unstable fps in v1.4.1x
				ldiff = ldiff + lvarGravityCorrection;
			},
			[&] { // rotation calculation
				RE::NiPoint3 InteriaMaxOffsetRot = {};
				RE::NiPoint3 InteriaMinOffsetRot = {};
				// when collisionElastic is 1 and collided, remove jitter caused by Max/Minoffsets
				if (multiplerInertiaRot >= 0.001f)
				{
					if (collisionInertiaRot.x > XmaxOffsetRot)
						InteriaMaxOffsetRot.x = collisionInertiaRot.x - XmaxOffsetRot;
					else if (collisionInertiaRot.x < XminOffsetRot)
						InteriaMinOffsetRot.x = collisionInertiaRot.x - XminOffsetRot;
					if (collisionInertiaRot.y > YmaxOffsetRot)
						InteriaMaxOffsetRot.y = collisionInertiaRot.y - YmaxOffsetRot;
					else if (collisionInertiaRot.y < YminOffsetRot)
						InteriaMinOffsetRot.y = collisionInertiaRot.y - YminOffsetRot;
					if (collisionInertiaRot.z > ZmaxOffsetRot)
						InteriaMaxOffsetRot.z = collisionInertiaRot.z - ZmaxOffsetRot;
					else if (collisionInertiaRot.z < ZminOffsetRot)
						InteriaMinOffsetRot.z = collisionInertiaRot.z - ZminOffsetRot;
					multiplerInertiaRot -= ((static_cast<float>(originalDeltaT) / timeTickRot) * 0.01f * timeStepRot);
					if (multiplerInertiaRot < 0.001f)
						multiplerInertiaRot = 0.0f;
					InteriaMaxOffsetRot *= multiplerInertiaRot;
					InteriaMinOffsetRot *= multiplerInertiaRot;
				}
				float timeMultiplierRot = timeTickRot / static_cast<float>(deltaTRot);
				diffRot = invRot * (diffRot * timeMultiplierRot);
				// linear X = rotation Y, linear Y = rotation Z, linear Z = rotation X
				RE::NiPoint3 stiffnessXYZRot(stiffnessYRot, stiffnessZRot, stiffnessXRot);
				RE::NiPoint3 stiffness2XYZRot(stiffness2YRot, stiffness2ZRot, stiffness2XRot);
				RE::NiPoint3 dampingXYZRot(dampingYRot, dampingZRot, dampingXRot);
				RE::NiPoint3 diff2Rot(diffRot.x * diffRot.x * sgn(diffRot.x), diffRot.y * diffRot.y * sgn(diffRot.y), diffRot.z * diffRot.z * sgn(diffRot.z));
				RE::NiPoint3 forceRot = (RE::NiPoint3((diffRot.x * stiffnessXYZRot.x) + (diff2Rot.x * stiffness2XYZRot.x)
				                                      , (diffRot.y * stiffnessXYZRot.y) + (diff2Rot.y * stiffness2XYZRot.y)
				                                      , (diffRot.z * stiffnessXYZRot.z) + (diff2Rot.z * stiffness2XYZRot.z))
					- forceGravityBias);
				velocityRot = invRot * velocityRot;
				do {
					// Assume mass is 1, so Accelleration is Force, can vary mass by changinf force
					//velocity = (velocity + (force * timeStep)) * (1 - (damping * timeStep));
					velocityRot = velocityRot * Friction; //Fixes that when becomes unstable collisions during colliding at low or unstable FPS
					velocityRot.x = (velocityRot.x + (forceRot.x * timeStepRot)) - (velocityRot.x * (dampingXYZRot.x * timeStepRot));
					velocityRot.y = (velocityRot.y + (forceRot.y * timeStepRot)) - (velocityRot.y * (dampingXYZRot.y * timeStepRot));
					velocityRot.z = (velocityRot.z + (forceRot.z * timeStepRot)) - (velocityRot.z * (dampingXYZRot.z * timeStepRot));
					posDeltaRot += velocityRot * timeStepRot;
					deltaTRot -= timeTickRot;
				} while (deltaTRot >= timeTickRot);
				velocityRot = obj->parent->world.rotate * velocityRot; //velocity is maintain on world transform
				newPosRot = newPosRot + obj->parent->world.rotate * (posDeltaRot * fpsCorrection);
				RE::NiPoint3 newdiffRot = newPosRot - target;
				varRotationalXnew = varRotationalXnew * forceAmplitude * amplitude * amplitude;
				varRotationalYnew = varRotationalYnew * forceAmplitude * amplitude * amplitude;
				varRotationalZnew = varRotationalZnew * forceAmplitude * amplitude * amplitude;
				ldiffRot = invRot * newdiffRot;
				auto beforenewrdiffRot = ldiffRot;
				ldiffRot.x = clamp(ldiffRot.x, YminOffsetRot + InteriaMinOffsetRot.x, YmaxOffsetRot + InteriaMaxOffsetRot.x); //rot y
				ldiffRot.y = clamp(ldiffRot.y, ZminOffsetRot + InteriaMinOffsetRot.y, ZmaxOffsetRot + InteriaMaxOffsetRot.y); //rot z
				ldiffRot.z = clamp(ldiffRot.z, XminOffsetRot + InteriaMinOffsetRot.z, XmaxOffsetRot + InteriaMaxOffsetRot.z); //rot x
				beforenewrdiffRot = beforenewrdiffRot - ldiffRot;
				ldiffRot.x = ldiffRot.x + ((beforenewrdiffRot.z * rotationXspreadforceY) + (beforenewrdiffRot.y * rotationZspreadforceY));
				ldiffRot.y = ldiffRot.y + ((beforenewrdiffRot.z * rotationXspreadforceZ) + (beforenewrdiffRot.x * rotationYspreadforceZ));
				ldiffRot.z = ldiffRot.z + ((beforenewrdiffRot.x * rotationYspreadforceX) + (beforenewrdiffRot.y * rotationZspreadforceX));
				ldiffRot.x = clamp(ldiffRot.x, YminOffsetRot + InteriaMinOffsetRot.x, YmaxOffsetRot + InteriaMaxOffsetRot.x); //rot y
				ldiffRot.y = clamp(ldiffRot.y, ZminOffsetRot + InteriaMinOffsetRot.y, ZmaxOffsetRot + InteriaMaxOffsetRot.y); //rot z
				ldiffRot.z = clamp(ldiffRot.z, XminOffsetRot + InteriaMinOffsetRot.z, XmaxOffsetRot + InteriaMaxOffsetRot.z); //rot x
				ldiffRot = ldiffRot + lvarGravityCorrection;

				auto rdiffXnew = ldiffRot * varRotationalXnew;
				auto rdiffYnew = ldiffRot * varRotationalYnew;
				auto rdiffZnew = ldiffRot * varRotationalZnew;

				rdiffXnew.x *= linearXrotationX;
				rdiffXnew.y *= linearYrotationX;
				rdiffXnew.z *= linearZrotationX; //1

				rdiffYnew.x *= linearXrotationY; //1
				rdiffYnew.y *= linearYrotationY;
				rdiffYnew.z *= linearZrotationY;

				rdiffZnew.x *= linearXrotationZ;
				rdiffZnew.y *= linearYrotationZ; //1
				rdiffZnew.z *= linearZrotationZ;

				newRot.SetEulerAnglesXYZ(rdiffYnew.x + rdiffYnew.y + rdiffYnew.z, rdiffZnew.x + rdiffZnew.y + rdiffZnew.z, rdiffXnew.x + rdiffXnew.y + rdiffXnew.z);
			});

		///#### physics calculate done
		///#### collision calculate start

		RE::NiPoint3 ldiffcol = {};
		RE::NiPoint3 ldiffGcol = {};
		RE::NiPoint3 maybeIdiffcol = {};

		if (collisionsOn && ActorCollisionsEnabled)
		{
			std::vector<int> thingIdList;
			std::vector<int> hashIdList;
			RE::NiPoint3 GroundCollisionVector = {};

			//The rotation of the previous frame due to collisions should not be used
			RE::NiMatrix3 objRotation = obj->parent->world.rotate * thingDefaultRot * newRot;

			LOG("Before Maybe Collision Stuff Start");
			auto maybeldiff = ldiff;
			maybeldiff.x = maybeldiff.x * varLinearX;
			maybeldiff.y = maybeldiff.y * varLinearY;
			maybeldiff.z = maybeldiff.z * varLinearZ;

			RE::NiPoint3 playerPos = (RE::PlayerCharacter::GetSingleton())->loadedData->data3D->world * RE::NiPoint3(0.0f, cogOffset, 0.0f);
			RE::NiPoint3 maybePos = target + (obj->parent->world.rotate * (maybeldiff + (thingDefaultPos * nodeScale))); //add missing local pos

			float colliderNodescale = 1.0f - ((1.0f - (nodeScale / actorBaseScale)) * scaleWeight); //Calibrate the scale gap between collider and actual mesh caused by bone weight

			//After cbp movement collision detection
			for (int i = 0; i < thingCollisionSpheres.size(); i++)
			{
				thingCollisionSpheres[i].offset100 = thingCollisionSpheres[i].offset0 * actorBaseScale * colliderNodescale;
				thingCollisionSpheres[i].worldPos = maybePos + (objRotation * thingCollisionSpheres[i].offset100);
				thingCollisionSpheres[i].radius100 = thingCollisionSpheres[i].radius0 * actorBaseScale * colliderNodescale;
				thingCollisionSpheres[i].radius100pwr2 = thingCollisionSpheres[i].radius100 * thingCollisionSpheres[i].radius100;
				hashIdList = GetHashIdsFromPos(thingCollisionSpheres[i].worldPos - playerPos, thingCollisionSpheres[i].radius100);
				for (int m = 0; m < hashIdList.size(); m++)
				{
					if (std::ranges::find(thingIdList, hashIdList[m]) == thingIdList.end())
					{
						thingIdList.emplace_back(hashIdList[m]);
					}
				}
			}
			for (int i = 0; i < thingCollisionCapsules.size(); i++)
			{
				thingCollisionCapsules[i].End1_offset100 = thingCollisionCapsules[i].End1_offset0 * actorBaseScale * colliderNodescale;
				thingCollisionCapsules[i].End1_worldPos = maybePos + (objRotation * thingCollisionCapsules[i].End1_offset100);
				thingCollisionCapsules[i].End1_radius100 = thingCollisionCapsules[i].End1_radius0 * actorBaseScale * colliderNodescale;
				thingCollisionCapsules[i].End1_radius100pwr2 = thingCollisionCapsules[i].End1_radius100 * thingCollisionCapsules[i].End1_radius100;
				thingCollisionCapsules[i].End2_offset100 = thingCollisionCapsules[i].End2_offset0 * actorBaseScale * colliderNodescale;
				thingCollisionCapsules[i].End2_worldPos = maybePos + (objRotation * thingCollisionCapsules[i].End2_offset100);
				thingCollisionCapsules[i].End2_radius100 = thingCollisionCapsules[i].End2_radius0 * actorBaseScale * colliderNodescale;
				thingCollisionCapsules[i].End2_radius100pwr2 = thingCollisionCapsules[i].End2_radius100 * thingCollisionCapsules[i].End2_radius100;
				hashIdList = GetHashIdsFromPos((thingCollisionCapsules[i].End2_worldPos + thingCollisionCapsules[i].End2_worldPos) * 0.5f - playerPos
					, (thingCollisionCapsules[i].End1_radius100 + thingCollisionCapsules[i].End2_radius100) * 0.5f);
				for (int m = 0; m < hashIdList.size(); m++)
				{
					if (std::ranges::find(thingIdList, hashIdList[m]) == thingIdList.end())
					{
						thingIdList.emplace_back(hashIdList[m]);
					}
				}
			}

			//Prevent normal movement to cause collision (This prevents shakes)			
			collisionVector = {};
			RE::NiPoint3 lastcollisionVector = {};
			CollisionConfig.maybePos = maybePos;
			CollisionConfig.origRot = obj->parent->world.rotate;
			CollisionConfig.objRot = objRotation;
			CollisionConfig.invRot = invRot;
			//set by linear move base (If implement accurate collisions for rotation only later on then need to add a rotation version)
			RE::NiPoint3 Currentldiff = ldiff - lvarGravityCorrection;
			CollisionConfig.CollisionMaxOffset.x = collisionXmaxOffset - Currentldiff.x;
			CollisionConfig.CollisionMinOffset.x = collisionXminOffset - Currentldiff.x;
			CollisionConfig.CollisionMaxOffset.y = collisionYmaxOffset - Currentldiff.y;
			CollisionConfig.CollisionMinOffset.y = collisionYminOffset - Currentldiff.y;
			CollisionConfig.CollisionMaxOffset.z = collisionZmaxOffset - Currentldiff.z;
			CollisionConfig.CollisionMinOffset.z = collisionZminOffset - Currentldiff.z;
			if (CollisionConfig.CollisionMaxOffset.x < 0.0f)
				CollisionConfig.CollisionMaxOffset.x = 0.0f;
			if (CollisionConfig.CollisionMinOffset.x > 0.0f)
				CollisionConfig.CollisionMinOffset.x = 0.0f;
			if (CollisionConfig.CollisionMaxOffset.y < 0.0f)
				CollisionConfig.CollisionMaxOffset.y = 0.0f;
			if (CollisionConfig.CollisionMinOffset.y > 0.0f)
				CollisionConfig.CollisionMinOffset.y = 0.0f;
			if (CollisionConfig.CollisionMaxOffset.z < 0.0f)
				CollisionConfig.CollisionMaxOffset.z = 0.0f;
			if (CollisionConfig.CollisionMinOffset.z > 0.0f)
				CollisionConfig.CollisionMinOffset.z = 0.0f;
			for (int j = 0; j < thingIdList.size(); j++)
			{
				int id = thingIdList[j];
				LOG_INFO("Thing hashId={}", id);
				if (Globals::partitions.find(id) != Globals::partitions.end())
				{
					for (int i = 0; i < Globals::partitions[id].partitionCollisions.size(); i++)
					{
						if (IgnoreAllSelfColliders && Globals::partitions[id].partitionCollisions[i].colliderActor == actor)
						{
							continue;
						}
						if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && std::ranges::find(IgnoredSelfCollidersList, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != IgnoredSelfCollidersList.end())
						{
							continue;
						}

						if (std::ranges::find(IgnoredCollidersList, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != IgnoredCollidersList.end())
						{
							continue;
						}

						//Actor's own genitals are ignored
						if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && Globals::partitions[id].partitionCollisions[i].colliderNodeName.find("Genital") != std::string::npos)
							continue;

						//Actor's own same obj is ignored, of course
						if (Globals::partitions[id].partitionCollisions[i].colliderActor == actor && std::strcmp(Globals::partitions[id].partitionCollisions[i].colliderNodeName.c_str(), boneName.c_str()) == 0)
							continue;

						if (Globals::debugtimelog || Globals::logging)
							InterlockedIncrement(&Globals::callCount);

						Globals::partitions[id].partitionCollisions[i].CollidedWeight = actorWeight;

						bool colliding = false;
						colliding = Globals::partitions[id].partitionCollisions[i].CheckCollision(collisionVector, thingCollisionSpheres, thingCollisionCapsules, CollisionConfig, false, true);
						if (colliding)
						{
							maybeNot = true;

							if (Globals::partitions[id].partitionCollisions[i].colliderActor == (RE::PlayerCharacter::GetSingleton()) && std::ranges::find(Globals::PlayerCollisionEventNodes, Globals::partitions[id].partitionCollisions[i].colliderNodeName) != Globals::PlayerCollisionEventNodes.end())
							{
								Globals::ActorNodePlayerCollisionEventMap[actorNodeString].collisionInThisCycle = true;
							}
							if (Globals::partitions[id].partitionCollisions[i].colliderActor == (RE::PlayerCharacter::GetSingleton()) && Globals::partitions[id].partitionCollisions[i].colliderNodeName.find("Genital") != std::string::npos)
							{
								Globals::ActorNodePlayerGenitalCollisionEventMap[actorNodeString].collisionInThisCycle = true;
							}
						}

						collisionCheckCount++;
					}
				}
			}

			//ground collision	
			if (GroundCollisionEnabled)
			{
				float bottomPos = groundPos;
				float bottomRadius = 0.0f;

				for (int l = 0; l < thingCollisionSpheres.size(); l++)
				{
					if (thingCollisionSpheres[l].worldPos.z - thingCollisionSpheres[l].radius100 < bottomPos - bottomRadius)
					{
						bottomPos = thingCollisionSpheres[l].worldPos.z;
						bottomRadius = thingCollisionSpheres[l].radius100;
					}
				}

				for (int m = 0; m < thingCollisionCapsules.size(); m++)
				{

					if (thingCollisionCapsules[m].End1_worldPos.z - thingCollisionCapsules[m].End1_radius100 < thingCollisionCapsules[m].End2_worldPos.z - thingCollisionCapsules[m].End2_radius100)
					{
						if (thingCollisionCapsules[m].End1_worldPos.z - thingCollisionCapsules[m].End1_radius100 < bottomPos - bottomRadius)
						{
							bottomPos = thingCollisionCapsules[m].End1_worldPos.z;
							bottomRadius = thingCollisionCapsules[m].End1_radius100;
						}
					}
					else
					{
						if (thingCollisionCapsules[m].End2_worldPos.z - thingCollisionCapsules[m].End2_radius100 < bottomPos - bottomRadius)
						{
							bottomPos = thingCollisionCapsules[m].End2_worldPos.z;
							bottomRadius = thingCollisionCapsules[m].End2_radius100;
						}
					}
				}

				if (bottomPos - bottomRadius < groundPos)
				{
					maybeNot = true;

					float Scalar = groundPos - (bottomPos - bottomRadius);

					//it can allow only force up to the radius for doesn't get crushed by the ground
					if (Scalar > bottomRadius)
						Scalar = 0.0f;

					GroundCollisionVector = RE::NiPoint3(0.0f, 0.0f, Scalar);
				}
			}

			if (maybeNot)
			{
				collisionOnLastFrame = maybeNot;

				ldiffcol = invRot * (collisionVector * collisionPenetration);
				ldiffGcol = invRot * (GroundCollisionVector * collisionPenetration);

				//Add more collision force for weak bone weights but virtually for maintain collision by obj position
				//For example, if a obj has a bone weight value of about 0.1, that shape seems actually moves by 0.1 even if the obj moves by 1
				//However, simply applying the multipler then changes the actual obj position,so that's making the collisions out of sync
				//Therefore to make perfect collision
				//it seems to be pushed out as much as colliding to the naked eye, but the actual position of the colliding obj must be maintained original position
				maybeIdiffcol = ldiffcol + ldiffGcol;
				auto mayberdiffcol = maybeIdiffcol;

				//add collision vector buffer of one frame to some reduce jitter and add softness by collision
				//be particularly useful for both nodes colliding that defined in both affected and collider nodes
				auto maybeldiffcoltmp = maybeIdiffcol;
				maybeIdiffcol = (maybeIdiffcol + collisionBuffer) * 0.5f * collisionMultipler;
				mayberdiffcol = (mayberdiffcol + collisionBuffer) * 0.5f * collisionMultiplerRot;
				collisionBuffer = maybeldiffcoltmp;

				//set to collision sync for the obj that has both affected obj and collider obj
				collisionSync = obj->parent->world.rotate * (ldiffcol + ldiffGcol - maybeIdiffcol);

				auto rcoldiffXnew = mayberdiffcol * varRotationalXnew;
				auto rcoldiffYnew = mayberdiffcol * varRotationalYnew;
				auto rcoldiffZnew = mayberdiffcol * varRotationalZnew;

				rcoldiffXnew.x *= linearXrotationX;
				rcoldiffXnew.y *= linearYrotationX;
				rcoldiffXnew.z *= linearZrotationX; //1

				rcoldiffYnew.x *= linearXrotationY; //1
				rcoldiffYnew.y *= linearYrotationY;
				rcoldiffYnew.z *= linearZrotationY;

				rcoldiffZnew.x *= linearXrotationZ;
				rcoldiffZnew.y *= linearYrotationZ; //1
				rcoldiffZnew.z *= linearZrotationZ;

				RE::NiMatrix3 newcolRot;
				newcolRot.SetEulerAnglesXYZ(rcoldiffYnew.x + rcoldiffYnew.y + rcoldiffYnew.z, rcoldiffZnew.x + rcoldiffZnew.y + rcoldiffZnew.z, rcoldiffXnew.x + rcoldiffXnew.y + rcoldiffXnew.z);

				newRot = newRot * newcolRot;
			}
			else
			{
				maybeIdiffcol = collisionBuffer * 0.5f * collisionMultipler;
				auto mayberdiffcol = collisionBuffer * 0.5f * collisionMultiplerRot;
				collisionBuffer = {};
				collisionSync = {} - maybeIdiffcol;
				auto rcoldiffXnew = mayberdiffcol * varRotationalXnew;
				auto rcoldiffYnew = mayberdiffcol * varRotationalYnew;
				auto rcoldiffZnew = mayberdiffcol * varRotationalZnew;
				rcoldiffXnew.x *= linearXrotationX;
				rcoldiffXnew.y *= linearYrotationX;
				rcoldiffXnew.z *= linearZrotationX; //1
				rcoldiffYnew.x *= linearXrotationY; //1
				rcoldiffYnew.y *= linearYrotationY;
				rcoldiffYnew.z *= linearZrotationY;
				rcoldiffZnew.x *= linearXrotationZ;
				rcoldiffZnew.y *= linearYrotationZ; //1
				rcoldiffZnew.z *= linearZrotationZ;
				RE::NiMatrix3 newcolRot;
				newcolRot.SetEulerAnglesXYZ(rcoldiffYnew.x + rcoldiffYnew.y + rcoldiffYnew.z, rcoldiffZnew.x + rcoldiffZnew.y + rcoldiffZnew.z, rcoldiffXnew.x + rcoldiffXnew.y + rcoldiffXnew.z);
				newRot = newRot * newcolRot;
			}
			///#### collision calculate done

			LOG("After Maybe Collision Stuff End");
		}
		//the collision accuracy is now almost perfect except for the rotation
		//well, I don't have an idea to be performance-friendly about the accuracy of collisions rotation
		//

		//logger.error("set positions\n");

		//If put the result of collision into the next frame, the quality of collision and movement will improve
		//but if that part is almost exclusively for collisions like vagina, it's better not to reflect the result of collision into physics
		//### To be free from unstable FPS, have to remove the varGravityCorrection from the next frame
		if ((collisionElastic || (IsBellyBone && ActorCollisionsEnabled && thing_bellybulgemultiplier > 0.0f)) && maybeNot)
		{
			oldWorldPos = (obj->parent->world.rotate * (ldiff + ldiffcol + ldiffGcol)) + target - RE::NiPoint3(0.0f, 0.0f, varGravityCorrection);
			oldWorldPosRot = (obj->parent->world.rotate * (ldiffRot + (ldiffcol + ldiffGcol) * collisionMultiplerRot)) + target - RE::NiPoint3(0.0f, 0.0f, varGravityCorrection);
			//for update oldWorldPos&Rot when frame gap
			oldLocalPos = ldiff + ldiffcol + ldiffGcol - (invRot * RE::NiPoint3(0.0f, 0.0f, varGravityCorrection));
			oldLocalPosRot = (ldiffRot + (ldiffcol + ldiffGcol) * collisionMultiplerRot) - (invRot * RE::NiPoint3(0.0f, 0.0f, varGravityCorrection));

			if (collisionElasticConstraints)
			{
				collisionInertia = ldiff + (ldiffcol + ldiffGcol);
				collisionInertiaRot = ldiffRot + (ldiffcol + ldiffGcol) * collisionMultiplerRot;

				multiplerInertia = 1.0f;
				multiplerInertiaRot = 1.0f;
			}
		}
		else
		{
			oldWorldPos = (obj->parent->world.rotate * ldiff) + target - RE::NiPoint3(0.0f, 0.0f, varGravityCorrection);
			oldWorldPosRot = (obj->parent->world.rotate * ldiffRot) + target - RE::NiPoint3(0.0f, 0.0f, varGravityCorrection);

			//for update oldWorldPos&Rot when frame gap
			oldLocalPos = ldiff - lvarGravityCorrection;
			oldLocalPosRot = ldiffRot - lvarGravityCorrection;
		}

		RE::NiPoint3 otherOffsets;

		if (IsBellyBone && ActorCollisionsEnabled && thing_bellybulgemultiplier > 0.0f)
		{
			ApplyBellyBulge(actor, thing_ReadNode_lock, thing_SetNode_lock);

			// Smoothing
			if (oldBellyBulge != newBellyBulge)
			{
				if (newBellyBulge > oldBellyBulge) // Increasing
				{
					float bulgediff = newBellyBulge - oldBellyBulge;

					if (bulgediff < 0.01f)
						oldBellyBulge = newBellyBulge;
					else
						oldBellyBulge = fminf(oldBellyBulge + clamp(bulgediff / 2.0f, 0.1f, 2.5f * thing_bellyBulgeSpeed), newBellyBulge);
				}
				else // Decreasing
				{
					float bulgediff = oldBellyBulge - newBellyBulge;

					if (bulgediff < 0.01f)
						oldBellyBulge = newBellyBulge;
					else
						oldBellyBulge = fmaxf(oldBellyBulge - clamp(bulgediff / 3.0f, 0.1f, 1.0f * thing_bellyBulgeSpeed), newBellyBulge);
				}
			}

			if ((oldBellyBulge > 0.0f) || updateBellyBulge)
			{
				updateBellyBulge = false; // This is used to fix issues with cbpc reset

				//need to convert world pos diif to local pos diff due to node scale
				//but min/max limit follows node scale because it can cause like torn pussy in small actor
				float horPos = oldBellyBulge * thing_bellybulgemultiplier * nodeParentInvScale;
				horPos = clamp(horPos, 0.0f, thing_bellybulgemax);
				float lowPos = (thing_bellybulgeposlowest / thing_bellybulgemax) * horPos;


				if (lastMaxOffsetY < horPos)
				{
					lastMaxOffsetY = abs(horPos);
					lastMaxOffsetZ = abs(lowPos);
				}

				LOG("belly bulge vert:{} horiz:{}", lowPos, horPos);

				otherOffsets.y += horPos;
				otherOffsets.z += lowPos;
			}
		}

		thing_SetNode_lock.lock();
		obj->local.translate.x = thingDefaultPos.x + otherOffsets.x + XdefaultOffset + (((ldiff.x * varLinearX) + maybeIdiffcol.x) * nodeParentInvScale);
		obj->local.translate.y = thingDefaultPos.y + otherOffsets.y + YdefaultOffset + (((ldiff.y * varLinearY) + maybeIdiffcol.y) * nodeParentInvScale);
		obj->local.translate.z = thingDefaultPos.z + otherOffsets.z + ZdefaultOffset + (((ldiff.z * varLinearZ) + maybeIdiffcol.z) * nodeParentInvScale);
		obj->local.rotate = thingDefaultRot * newRot;
		RefreshNode(obj);
		thing_SetNode_lock.unlock();

	}

	void Thing::CalculateDiffVagina(RE::NiPoint3& collisionDiff, float opening, bool isleftandright, bool leftORback)
	{
		if (opening > 0)
		{
			if (isleftandright)
			{
				if (leftORback)
				{//left
					collisionDiff = RE::NiPoint3(thing_vaginaOpeningMultiplier * -1.0f, 0.0f, 0.0f) * (opening * 0.5f);
				}
				else
				{//right
					collisionDiff = RE::NiPoint3(thing_vaginaOpeningMultiplier, 0.0f, 0.0f) * (opening * 0.5f);
				}
			}
			else
			{
				if (leftORback)
				{//Back
					collisionDiff = RE::NiPoint3(0.0f, thing_vaginaOpeningMultiplier * -0.75f, thing_vaginaOpeningMultiplier * 0.25f) * (opening * 0.5f);
				}
				else
				{//front
					collisionDiff = RE::NiPoint3(0.0f, thing_vaginaOpeningMultiplier * 0.125f, thing_vaginaOpeningMultiplier * -0.25f) * (opening * 0.5f);
				}
			}
		}
		else
		{
			collisionDiff = {};
		}
	}
	void Thing::CalculateDiffAnus(RE::NiPoint3& collisionDiff, float opening, bool isleftandright, bool leftORback)
	{
		if (opening > 0.0f)
		{
			if (isleftandright)
			{
				if (leftORback)
				{//left
					collisionDiff = RE::NiPoint3(0.0f, thing_anusOpeningMultiplier * 1.0f, 0.0f) * (opening * 2);
				}
				else
				{//right
					collisionDiff = RE::NiPoint3(0.0f, thing_anusOpeningMultiplier * -1.0f, 0.0f) * (opening * 2);
				}
			}
			else
			{
				if (leftORback)
				{//back
					collisionDiff = RE::NiPoint3(thing_anusOpeningMultiplier * 0.8f, 0.0f, thing_anusOpeningMultiplier * 0.32f) * (opening * 2.0f);
				}
				else
				{//front
					collisionDiff = RE::NiPoint3(thing_anusOpeningMultiplier * -0.8f, 0.0f, thing_anusOpeningMultiplier * -0.32f) * (opening * 2.0f);
				}
			}
		}
		else
		{
			collisionDiff = {};
		}
	}


}
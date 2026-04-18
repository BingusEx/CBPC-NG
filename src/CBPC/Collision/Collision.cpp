#include "CBPC/Collision/Collision.hpp"
#include "CBPC/Globals.hpp"

namespace CBP {

	Collision::Collision(RE::NiAVObject* node, std::vector<Sphere>& colliderSpheres, std::vector<Capsule>& collidercapsules, float actorWeight) : colliderActor(nullptr) {
		
		CollisionObject = node;
		ColliderWeight = actorWeight;
		collisionSpheres = colliderSpheres;

		for (int j = 0; j < collisionSpheres.size(); j++) {
			collisionSpheres[j].offset0 = GetPointFromPercentage(colliderSpheres[j].offset0, colliderSpheres[j].offset100, ColliderWeight);
			collisionSpheres[j].radius0 = GetPercentageValue(colliderSpheres[j].radius0, colliderSpheres[j].radius100, ColliderWeight);
			collisionSpheres[j].radius100pwr2 = collisionSpheres[j].radius0 * collisionSpheres[j].radius0;
			collisionSpheres[j].NodeName = colliderSpheres[j].NodeName;
		}

		collisionCapsules = collidercapsules;
		for (int k = 0; k < collisionCapsules.size(); k++) {
			collisionCapsules[k].End1_offset0 = GetPointFromPercentage(collidercapsules[k].End1_offset0, collidercapsules[k].End1_offset100, ColliderWeight);
			collisionCapsules[k].End1_radius0 = GetPercentageValue(collidercapsules[k].End1_radius0,
			                                                       collidercapsules[k].End1_radius100, ColliderWeight);
			collisionCapsules[k].End1_radius100pwr2 = collisionCapsules[k].End1_radius0 * collisionCapsules[k].
				End1_radius0;
			collisionCapsules[k].End2_offset0 = GetPointFromPercentage(collidercapsules[k].End2_offset0,
			                                                           collidercapsules[k].End2_offset100,
			                                                           ColliderWeight);
			collisionCapsules[k].End2_radius0 = GetPercentageValue(collidercapsules[k].End2_radius0,
			                                                       collidercapsules[k].End2_radius100, ColliderWeight);
			collisionCapsules[k].End2_radius100pwr2 = collisionCapsules[k].End2_radius0 * collisionCapsules[k].
				End2_radius0;
			collisionCapsules[k].NodeName = collidercapsules[k].NodeName;
		}
	}

	void Collision::UpdateThingColliderPositions(RE::NiPoint3& Collisiondif, std::vector<Sphere>& thingCollisionSpheres, std::vector<Capsule>& thingCollisionCapsules, const CollisionConfigs& CollisionConfig) {
		auto lcollisiondif = CollisionConfig.invRot * Collisiondif;

		lcollisiondif.x = clamp(lcollisiondif.x, CollisionConfig.CollisionMinOffset.x, CollisionConfig.CollisionMaxOffset.x);
		lcollisiondif.y = clamp(lcollisiondif.y, CollisionConfig.CollisionMinOffset.y, CollisionConfig.CollisionMaxOffset.y);
		lcollisiondif.z = clamp(lcollisiondif.z, CollisionConfig.CollisionMinOffset.z, CollisionConfig.CollisionMaxOffset.z);

		Collisiondif = CollisionConfig.origRot * lcollisiondif;

		for (int l = 0; l < thingCollisionSpheres.size(); l++) {
			thingCollisionSpheres[l].worldPos = CollisionConfig.maybePos + (CollisionConfig.objRot * thingCollisionSpheres[l].offset100) + Collisiondif;
		}

		for (int m = 0; m < thingCollisionCapsules.size(); m++) {
			thingCollisionCapsules[m].End1_worldPos = CollisionConfig.maybePos + (CollisionConfig.objRot * thingCollisionCapsules[m].End1_offset100) + Collisiondif;
			thingCollisionCapsules[m].End2_worldPos = CollisionConfig.maybePos + (CollisionConfig.objRot * thingCollisionCapsules[m].End2_offset100) + Collisiondif;
		}
	}

	bool Collision::IsItColliding(RE::NiPoint3& collisiondif, std::vector<Sphere>& thingCollisionSpheres, std::vector<Sphere>& collisionSpheres, std::vector<Capsule>& thingCollisionCapsules, std::vector<Capsule>& collisionCapsules, CollisionConfigs CollisionConfig, bool maybe, bool additive, bool moveColliders) {

		bool isItColliding = false;

		for (int j = 0; j < thingCollisionSpheres.size(); j++) {
			RE::NiPoint3 thingSpherePosition = thingCollisionSpheres[j].worldPos;

			//Sphere x Sphere
			for (int i = 0; i < collisionSpheres.size(); i++) {
				float limitDistance = thingCollisionSpheres[j].radius100 + collisionSpheres[i].radius100;

				RE::NiPoint3 colSpherePosition = collisionSpheres[i].worldPos;

				float currentDistancePwr2 = distanceNoSqrt(thingSpherePosition, colSpherePosition);

				if (currentDistancePwr2 < limitDistance * limitDistance)
				{
					isItColliding = true;
					if (maybe)
						return true;

					float currentDistance = std::sqrt(currentDistancePwr2);
					double Scalar = limitDistance - currentDistance; //Get vector scalar

					RE::NiPoint3 thisCollision = GetVectorFromCollision(colSpherePosition, thingSpherePosition, Scalar, currentDistance); //Get collision vector

					if (additive)
					{
						collisiondif += thisCollision;
					}
					else
					{
						if (distanceNoSqrt(collisiondif, {}) < distanceNoSqrt(thisCollision, {}))
						{
							collisiondif = thisCollision;
						}
					}
					if (moveColliders)
						UpdateThingColliderPositions(collisiondif, thingCollisionSpheres, thingCollisionCapsules, CollisionConfig);

				}
			}

			//sphere X capsule
			for (int k = 0; k < collisionCapsules.size(); k++)  {
				RE::NiPoint3 bestPoint = ClosestPointOnLineSegment(collisionCapsules[k].End1_worldPos, collisionCapsules[k].End2_worldPos, thingCollisionSpheres[j].worldPos);

				float twoPointDistancePwr2 = distanceNoSqrt(collisionCapsules[k].End1_worldPos, collisionCapsules[k].End2_worldPos);
				float bestPointDistancePwr2 = distanceNoSqrt(collisionCapsules[k].End1_worldPos, bestPoint);
				float PointWeight = bestPointDistancePwr2 / twoPointDistancePwr2 * 100;
				float limitDistance = GetPercentageValue(collisionCapsules[k].End1_radius100, collisionCapsules[k].End2_radius100, PointWeight) + thingCollisionSpheres[j].radius100;
				float currentDistancePwr2 = distanceNoSqrt(thingCollisionSpheres[j].worldPos, bestPoint);

				if (currentDistancePwr2 < limitDistance * limitDistance) {
					isItColliding = true;
					if (maybe) return true;

					float currentDistance = std::sqrt(currentDistancePwr2);
					double Scalar = limitDistance - currentDistance; //Get vector scalar

					RE::NiPoint3 thisCollision = GetVectorFromCollision(bestPoint, thingCollisionSpheres[j].worldPos, Scalar, currentDistance); //Get collision vector

					if (additive){
						collisiondif += thisCollision;
					}
					else {
						if (distanceNoSqrt(collisiondif, {}) < distanceNoSqrt(thisCollision, {})) {
							collisiondif = thisCollision;
						}
					}
					if (moveColliders) UpdateThingColliderPositions(collisiondif, thingCollisionSpheres, thingCollisionCapsules, CollisionConfig);
				}
			}
		}

		for (int n = 0; n < thingCollisionCapsules.size(); n++)
		{
			for (int l = 0; l < collisionSpheres.size(); l++) //capsule X sphere
			{
				RE::NiPoint3 bestPoint = ClosestPointOnLineSegment(thingCollisionCapsules[n].End1_worldPos, thingCollisionCapsules[n].End2_worldPos, collisionSpheres[l].worldPos);

				float twoPointDistancePwr2 = distanceNoSqrt(thingCollisionCapsules[n].End1_worldPos, thingCollisionCapsules[n].End2_worldPos);
				float bestPointDistancePwr2 = distanceNoSqrt(thingCollisionCapsules[n].End1_worldPos, bestPoint);
				float PointWeight = bestPointDistancePwr2 / twoPointDistancePwr2 * 100;
				float limitDistance = GetPercentageValue(thingCollisionCapsules[n].End1_radius100, thingCollisionCapsules[n].End2_radius100, PointWeight) + collisionSpheres[l].radius100;
				float currentDistancePwr2 = distanceNoSqrt(bestPoint, collisionSpheres[l].worldPos);

				if (currentDistancePwr2 < limitDistance * limitDistance) {
					isItColliding = true;
					if (maybe) return true;

					float currentDistance = std::sqrt(currentDistancePwr2);
					double Scalar = limitDistance - currentDistance; //Get vector scalar

					RE::NiPoint3 thisCollision = GetVectorFromCollision(collisionSpheres[l].worldPos, bestPoint, Scalar, currentDistance); //Get collision vector

					if (additive) {
						collisiondif += thisCollision;
					}
					else {
						if (distanceNoSqrt(collisiondif, {}) < distanceNoSqrt(thisCollision, {})) {
							collisiondif = thisCollision;
						}
					}
					if (moveColliders)
						UpdateThingColliderPositions(collisiondif, thingCollisionSpheres, thingCollisionCapsules, CollisionConfig);

				}
			}

			//capsule X capsule
			for (int m = 0; m < collisionCapsules.size(); m++){
				RE::NiPoint3 v0 = collisionCapsules[m].End1_worldPos - thingCollisionCapsules[n].End1_worldPos;
				RE::NiPoint3 v1 = collisionCapsules[m].End2_worldPos - thingCollisionCapsules[n].End1_worldPos;
				RE::NiPoint3 v2 = collisionCapsules[m].End1_worldPos - thingCollisionCapsules[n].End2_worldPos;
				RE::NiPoint3 v3 = collisionCapsules[m].End2_worldPos - thingCollisionCapsules[n].End2_worldPos;

				float d0 = Dot(v0, v0);
				float d1 = Dot(v1, v1);
				float d2 = Dot(v2, v2);
				float d3 = Dot(v3, v3);

				RE::NiPoint3 bestPointA;
				if (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1){
					bestPointA = thingCollisionCapsules[n].End2_worldPos;
				}
				else{
					bestPointA = thingCollisionCapsules[n].End1_worldPos;
				}

				RE::NiPoint3 bestPointB = ClosestPointOnLineSegment(collisionCapsules[m].End1_worldPos, collisionCapsules[m].End2_worldPos, bestPointA);
				bestPointA = ClosestPointOnLineSegment(thingCollisionCapsules[n].End1_worldPos, thingCollisionCapsules[n].End2_worldPos, bestPointB);

				float twoPointADistancePwr2 = distanceNoSqrt(thingCollisionCapsules[n].End1_worldPos, thingCollisionCapsules[n].End2_worldPos);
				float twoPointBDistancePwr2 = distanceNoSqrt(collisionCapsules[m].End1_worldPos, collisionCapsules[m].End2_worldPos);

				float bestPointADistancePwr2 = distanceNoSqrt(thingCollisionCapsules[n].End1_worldPos, bestPointA);
				float bestPointBDistancePwr2 = distanceNoSqrt(collisionCapsules[m].End1_worldPos, bestPointB);

				float PointAWeight = bestPointADistancePwr2 / twoPointADistancePwr2 * 100;
				float PointBWeight = bestPointBDistancePwr2 / twoPointBDistancePwr2 * 100;

				float limitDistance = GetPercentageValue(thingCollisionCapsules[n].End1_radius100, thingCollisionCapsules[n].End2_radius100, PointAWeight) + GetPercentageValue(collisionCapsules[m].End1_radius100, collisionCapsules[m].End2_radius100, PointBWeight);

				float currentDistancePwr2 = distanceNoSqrt(bestPointA, bestPointB);

				if (currentDistancePwr2 < limitDistance * limitDistance) {
					isItColliding = true;
					if (maybe) return true;

					float currentDistance = std::sqrt(currentDistancePwr2);
					double Scalar = limitDistance - currentDistance; //Get vector scalar

					RE::NiPoint3 thisCollision = GetVectorFromCollision(bestPointB, bestPointA, Scalar, currentDistance); //Get collision vector

					if (additive){
						collisiondif += thisCollision;
					}
					else {
						if (distanceNoSqrt(collisiondif, {}) < distanceNoSqrt(thisCollision, {})) {
							collisiondif = thisCollision;
						}
					}
					if (moveColliders)
						UpdateThingColliderPositions(collisiondif, thingCollisionSpheres, thingCollisionCapsules, CollisionConfig);

				}
			}
		}

		return isItColliding;
	}

	bool Collision::CheckPelvisCollision(RE::NiPoint3& collisionDiff, std::vector<Sphere>& thingCollisionSpheres, std::vector<Capsule>& thingCollisionCapsules, const CollisionConfigs& CollisionConfig, bool additive)
	{
		

		auto IsColliding = false;

		if (CollisionObject != nullptr)
		{
			IsColliding = IsItColliding(collisionDiff, thingCollisionSpheres, collisionSpheres, thingCollisionCapsules, collisionCapsules, CollisionConfig, false, additive, false);

		}


		return IsColliding;
	}

	bool Collision::CheckCollision(RE::NiPoint3& collisionDiff, std::vector<Sphere>& thingCollisionSpheres, std::vector<Capsule>& thingCollisionCapsules, const CollisionConfigs& CollisionConfig, bool maybe, bool additive)
	{

		bool isColliding = false;
		if (CollisionObject != nullptr)
		{
			isColliding = IsItColliding(collisionDiff, thingCollisionSpheres, collisionSpheres, thingCollisionCapsules, collisionCapsules, CollisionConfig, maybe, additive, true);
			if (isColliding)
			{
				if (maybe)
					return true;
			}
		}

		return isColliding;
	}

	// Dot product of 2 vectors 
	float Collision::Dot(RE::NiPoint3 A, RE::NiPoint3 B) {
		float x1 = A.x * B.x;
		float y1 = A.y * B.y;
		float z1 = A.z * B.z;
		return (x1 + y1 + z1);
	}

	RE::NiPoint3 Collision::ClosestPointOnLineSegment(RE::NiPoint3 lineStart, RE::NiPoint3 lineEnd, RE::NiPoint3 point) {
		auto v = lineEnd - lineStart;
		auto t = Dot(point - lineStart, v) / Dot(v, v);
		t = std::max(t, 0.0f);
		t = std::min(t, 1.0f);
		return lineStart + v * t;
	}

	Collision::~Collision() {}

}

#pragma once
#include "CBPC/Collision/Shapes.hpp"

 namespace CBP {

	 struct CollisionConfigs {

		 bool IsElasticCollision = false;

		 RE::NiPoint3 maybePos = {};

		 RE::NiMatrix3 objRot;
		 RE::NiMatrix3 origRot;
		 RE::NiMatrix3 invRot;

		 RE::NiPoint3 CollisionMaxOffset = { -100.0f, -100.0f, -100.0f };
		 RE::NiPoint3 CollisionMinOffset =  { -100.0f, -100.0f, -100.0f };
	 };

	 class Collision {

		public:

		 Collision(RE::NiAVObject* node, std::vector<Sphere>& colliderSpheres, std::vector<Capsule>& collidercapsules, float actorWeight);
		 ~Collision();

		 float CollidedWeight = 50;
		 float ColliderWeight = 50;

		 float actorBaseScale = 1.0f;

		 float scaleWeight = 1.0f;

		 RE::Actor* colliderActor;
		 RE::NiPoint3 lastColliderPosition = {};
		 std::vector<Sphere> collisionSpheres;
		 std::vector<Capsule> collisionCapsules;
		 RE::NiAVObject* CollisionObject;
		 std::string colliderNodeName;

		 static bool IsItColliding(RE::NiPoint3& collisiondif, std::vector<Sphere>& thingCollisionSpheres, std::vector<Sphere>& collisionSpheres, std::vector<Capsule>& thingCollisionCapsules, std::vector<Capsule>& collisionCapsules, CollisionConfigs CollisionConfig, bool maybe, bool additive, bool moveColliders);
		 bool CheckCollision(RE::NiPoint3& collisionDiff, std::vector<Sphere>& thingCollisionSpheres, std::vector<Capsule>& thingCollisionCapsules, const CollisionConfigs& CollisionConfig, bool maybe, bool additive);
		 bool CheckPelvisCollision(RE::NiPoint3& collisionDiff, std::vector<Sphere>& thingCollisionSpheres, std::vector<Capsule>& thingCollisionCapsules, const CollisionConfigs& CollisionConfig, bool additive);
		 static float Dot(RE::NiPoint3 A, RE::NiPoint3 B);
		 static RE::NiPoint3 ClosestPointOnLineSegment(RE::NiPoint3 lineStart, RE::NiPoint3 lineEnd, RE::NiPoint3 point);
		 static void UpdateThingColliderPositions(RE::NiPoint3& Collisiondif, std::vector<Sphere>& thingCollisionSpheres, std::vector<Capsule>& thingCollisionCapsules, const
		                                          CollisionConfigs& CollisionConfig);

	 };


 }

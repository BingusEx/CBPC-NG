#pragma once
#include "Collision/Collision.hpp"
#include "CBPC/Thing.hpp"

namespace CBP {

	class SimObj {
		
		public:
		tbb::concurrent_unordered_map<std::string, tbb::concurrent_unordered_map<std::string, Thing>> m_Things;
		tbb::concurrent_unordered_map<std::string, Collision> m_ActorColliders;
		tbb::concurrent_unordered_map<std::string, RE::NiPoint3> m_NodeCollisionSync;
		RE::Actor* m_OwnerActor;
		float m_ActorDistanceSqr;
		bool m_EnableGroundCollisions = false;
		float m_GroundPos = -10000.0f;

		SimObj(RE::Actor* a_owner);
		SimObj();
		~SimObj();

		bool Bind(RE::Actor* a_actor, bool a_isMale);
		bool UpdateConfig(RE::Actor* a_owner);
		void Update(RE::Actor* a_owner, bool a_CollisionsEnabled, bool a_IsFemale);
		void UpdateCollisions(RE::Actor* a_owner);
		bool IsBound() const { return Bound; }

		private:
		uint32_t ID = 0;
		bool Bound = false;

	};
}

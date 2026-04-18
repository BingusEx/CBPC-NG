#pragma once

namespace CBP {

	struct Sphere {
		RE::NiPoint3 offset0 = {};
		RE::NiPoint3 offset100 = {};
		double radius0 = 4.0;
		double radius100 = 4.0;
		double radius100pwr2 = 16.0;
		RE::NiPoint3 worldPos = {};
		std::string NodeName;
		uint32_t index = -1;

		bool operator==(const Sphere& other) const {
			return offset0.x == other.offset0.x && 
				   offset0.y == other.offset0.y && 
				   offset0.z == other.offset0.z && 
				   radius0 == other.radius0;
		}
	};

	struct Capsule
	{
		RE::NiPoint3 End1_offset0 = {};
		RE::NiPoint3 End1_offset100 = {};
		RE::NiPoint3 End1_worldPos = {};
		double End1_radius0 = 4.0;
		double End1_radius100 = 4.0;
		double End1_radius100pwr2 = 16.0;
		RE::NiPoint3 End2_offset0 = {};
		RE::NiPoint3 End2_offset100 = {};
		RE::NiPoint3 End2_worldPos = {};
		double End2_radius0 = 4.0;
		double End2_radius100 = 4.0;
		double End2_radius100pwr2 = 16.0;

		std::string NodeName;
		uint32_t index = -1;

		bool operator==(const Capsule& other) const {
			return 
				End1_offset0.x == other.End1_offset0.x && 
				End1_offset0.y == other.End1_offset0.y && 
				End1_offset0.z == other.End1_offset0.z &&
				End2_offset0.x == other.End2_offset0.x && 
				End2_offset0.y == other.End2_offset0.y && 
				End2_offset0.z == other.End2_offset0.z &&
				End1_radius0 == other.End1_radius0 && 
				End2_radius0 == other.End2_radius0;
		}
	};
}
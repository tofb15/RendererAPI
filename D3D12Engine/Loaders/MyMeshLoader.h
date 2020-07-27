#pragma once
#include <unordered_map>
#include "Math.hpp"

namespace LOADER {
	namespace MyMeshLoader {
		typedef std::unordered_map<std::string, std::vector<Float3>> FLOAT3_BUFFER;
		typedef std::unordered_map<std::string, std::vector<Float2>> FLOAT2_BUFFER;
		typedef std::unordered_map<std::string, std::vector<UINT>>   INT_BUFFER;

		bool LoadMesh(const char* fileName, FLOAT3_BUFFER& facePos, FLOAT3_BUFFER& faceNorms, FLOAT2_BUFFER& faceUVs);
		bool SaveMesh(const char* fileName, FLOAT3_BUFFER& facePos, FLOAT3_BUFFER& faceNorms, FLOAT2_BUFFER& faceUVs);
	}
}
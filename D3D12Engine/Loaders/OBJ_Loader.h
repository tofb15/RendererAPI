#pragma once
#include <string.h>
#include <vector>
#include <sstream>
#include <unordered_map>
#include "../Math.hpp"

namespace LOADER {
	typedef std::unordered_map<std::string, std::vector<Float3>> FLOAT3_BUFFER;
	typedef std::unordered_map<std::string, std::vector<Float2>> FLOAT2_BUFFER;

	bool LoadOBJ(const char* fileName, FLOAT3_BUFFER& facePos, FLOAT3_BUFFER& faceNorms, FLOAT2_BUFFER& faceUVs);
	bool SaveOBJ(const char* fileName, FLOAT3_BUFFER& facePos, FLOAT3_BUFFER& faceNorms, FLOAT2_BUFFER& faceUVs);
}
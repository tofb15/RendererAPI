#pragma once
#include <string.h>
#include <vector>
#include <sstream>
#include <unordered_map>
#include "../Utills/Math.hpp"

namespace LOADER {
	typedef std::unordered_map<std::string, std::vector<Float3>> FLOAT3_BUFFER;
	typedef std::unordered_map<std::string, std::vector<Float2>> FLOAT2_BUFFER;
	typedef std::unordered_map<std::string, std::vector<UINT>>   INT_BUFFER;

	bool LoadOBJ(const char* fileName, FLOAT3_BUFFER& facePos, FLOAT3_BUFFER& faceNorms, FLOAT2_BUFFER& faceUVs);
	/*
		LOAD using indexBuffer. This is not done so dont use this.
	*/
	bool LoadOBJ(const char* fileName, std::vector<Float3>& allPos, std::vector<Float2>& allUV, INT_BUFFER& faceIndex, FLOAT3_BUFFER& faceNorms);
	bool SaveOBJ(const char* fileName, FLOAT3_BUFFER& facePos, FLOAT3_BUFFER& faceNorms, FLOAT2_BUFFER& faceUVs);
}
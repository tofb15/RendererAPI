#ifdef HLSL
// Shader only

#define MERGE(a, b) a##b

#define PI 3.14159265359
#define M_1_PI 0.318309886183790671538

#else
// C++ only
#include <DirectXMath.h>
#pragma once

typedef DirectX::XMFLOAT2 float2;
typedef Float3 float3;
typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT3X3 float3x3;
typedef DirectX::XMFLOAT4X4 float4x4;
typedef DirectX::XMUINT2 uint2;
typedef DirectX::XMUINT3 uint3;
typedef unsigned int uint;

namespace DXRShaderCommon {
#endif

	static const int MAX_LIGHTS = 16;
	static const int N_RAY_TYPES = 2;
	static const unsigned int MAX_RAY_RECURSION_DEPTH = 10;
	static const unsigned int HIT_BY_PRIMARY_RAYS_FLAG = 0x1;
	static const unsigned int CASTING_SHADOW_FLAG = 0x2;
	

	struct RayPayload {
		float4 color;
		uint recursionDepth;
		float hitT;
	};

	struct RayPayload_shadow {
		uint inShadow;
	};

	struct PointLight {
		float3 position;
		float reachRadius;
		float3 color;	
		float padding2;
	};




	struct SceneCBuffer {
		float4x4 projectionToWorld; //Used for raygeneration
		float4x4 viewToWorld;
		float3 cameraPosition;
		int nLights;
		PointLight pLight[MAX_LIGHTS];

		/*
	   float4x4 padding;
	   float3x3 padding;
	   float4 padding;
	   */
	};

#ifndef HLSL
	// C++ only
} // End namespace
#endif


/////////////////////////////////////////////////////////////
//Standard HLSL things

#ifdef HLSL
static const uint g_SHADOW_RAY_FLAGS = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
static const float RAY_T_MAX = 2000.0f;

// Barycentric interpolation
float2 barrypolation(float3 barry, float2 in1, float2 in2, float2 in3) {
	return barry.x * in1 + barry.y * in2 + barry.z * in3;
}
float3 barrypolation(float3 barry, float3 in1, float3 in2, float3 in3) {
	return barry.x * in1 + barry.y * in2 + barry.z * in3;
}

float3 HitWorldPosition() {
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

//===Global Signature===
RaytracingAccelerationStructure gAS : register(t0, space0);
RaytracingAccelerationStructure gAS_alpha : register(t0, space1);

ConstantBuffer<SceneCBuffer> CB_SceneData : register(b0, space0);
sampler samp : register(s0);

struct TanBinorm {
	float3 tangent;
	float3 binormal;
};
//===Raygen Signature===
RWTexture2D<float4> outputTexture : register(u1);

//===ClosestHit Signature===
StructuredBuffer<float3> vertices_pos : register(t1, space0);
StructuredBuffer<float3> vertices_normal : register(t1, space1);
StructuredBuffer<float2> vertices_uv : register(t1, space2);
StructuredBuffer<TanBinorm> vertices_tan_bi : register(t1, space3);

#define ALBEDO_TEX_POS 0
#define NORMAL_TEX_POS 1
#define ROUGHNESS_TEX_POS 2
#define METAL_TEX_POS 3

Texture2D<float4> sys_textures[] : register(t2, space0);

bool PointInShadow(float3 rayOrigin, float3 directionToLight, float distToLight){
	//Check if the light is ocluded.
	RayDesc shadowRay;
	shadowRay.Origin = rayOrigin; 		
	shadowRay.Direction = directionToLight; 
	shadowRay.TMin = 0.00001;
	shadowRay.TMax = distToLight;
	RayPayload_shadow shadowPayload;
	shadowPayload.inShadow = 1;
	TraceRay(gAS, g_SHADOW_RAY_FLAGS, CASTING_SHADOW_FLAG, 1, N_RAY_TYPES, 1, shadowRay, shadowPayload);

	return shadowPayload.inShadow == 1;
}
#endif
#ifdef HLSL
// Shader only

#define MERGE(a, b) a##b

#define PI 3.14159265359
#define M_1_PI 0.318309886183790671538

#else
// C++ only
#pragma once

typedef DirectX::XMFLOAT2 float2;
typedef Float3 float3;
typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT3X3 float3x3;
typedef DirectX::XMFLOAT4X4 float4x4;
typedef DirectX::XMUINT2 uint2;
typedef DirectX::XMUINT3 uint3;
typedef unsigned int uint;

namespace DXRShaderCommon
{
#endif

struct RayPayload
{
	float4 color;
	uint recursionDepth;
};

struct SceneCBuffer
{
    float4x4 projectionToWorld; //Used for raygeneration
    float4x4 viewToWorld;
    float3 cameraPosition;
};

#ifndef HLSL
// C++ only
} // End namespace
#endif
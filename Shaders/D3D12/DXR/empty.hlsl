#define HLSL
#include "Common_hlsl_cpp.hlsli"
// Barycentric interpolation
float2 barrypolation(float3 barry, float2 in1, float2 in2, float2 in3)
{
    return barry.x * in1 + barry.y * in2 + barry.z * in3;
}
float3 barrypolation(float3 barry, float3 in1, float3 in2, float3 in3)
{
    return barry.x * in1 + barry.y * in2 + barry.z * in3;
}

float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

//===Global Signature===
RaytracingAccelerationStructure gAS : register(t0);
ConstantBuffer<SceneCBuffer> CB_SceneData : register(b0, space0);
sampler samp : register(s0);

struct TanBinorm
{
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

Texture2D<float4> sys_texAlbedo : register(t2, space0);
Texture2D<float4> sys_texNormMap : register(t2, space1);

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void generateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

	// Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 1, 1), transpose(CB_SceneData.projectionToWorld));

    world.xyz /= world.w;
    origin = CB_SceneData.cameraPosition.xyz;
    direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void rayGen() {
	uint2 launchIndex = DispatchRaysIndex().xy;

	RayDesc ray;
    generateCameraRay(launchIndex, ray.Origin, ray.Direction);
	ray.TMin = 0.00001;
	ray.TMax = 2000.0;

	RayPayload payload;
    payload.recursionDepth = 0;
	TraceRay(gAS, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);
    //float3 camPos = CB_SceneData.cameraPosition;
	////outputTexture[launchIndex] = float4(launchIndex.x / 64.0f, launchIndex.y / 64.0f, 0.0f, 1.0f);  
    outputTexture[launchIndex] = payload.color; //float4(camPos.x, camPos.y, camPos.z, 0);
}

[shader("closesthit")]
void closestHitTriangle(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
    float t = pow((1000 - RayTCurrent()) / 1000.f, 4);
    payload.color = float4(t, t, t, 1.0f);
}

[shader("miss")]
void miss(inout RayPayload payload) {
	payload.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
}
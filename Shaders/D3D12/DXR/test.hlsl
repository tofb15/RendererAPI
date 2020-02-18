#define HLSL
#include "Common_hlsl_cpp.hlsli"

static const uint g_SHADOW_RAY_FLAGS = RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
static const float RAY_T_MAX = 2000.0f;

// Barycentric interpolation
float2 barrypolation(float3 barry, float2 in1, float2 in2, float2 in3)
{
	return barry.x* in1 + barry.y * in2 + barry.z * in3;
}
float3 barrypolation(float3 barry, float3 in1, float3 in2, float3 in3)
{
	return barry.x* in1 + barry.y * in2 + barry.z * in3;
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

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void generateCameraRay2(uint2 index, out float3 origin, out float3 direction, out float3 origin2, out float3 direction2)
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

	float3 left = normalize(cross(direction, float3(0, 1, 0))) * 10.0f;
	
	origin2 = origin + left;
	direction2 = normalize(world.xyz - origin2);
	origin = origin - left;
	direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void rayGen() {
	uint2 launchIndex = DispatchRaysIndex().xy;

	RayDesc ray[2];

	//generateCameraRay2(launchIndex, ray[0].Origin, ray[0].Direction, ray[1].Origin, ray[1].Direction);
	generateCameraRay(launchIndex, ray[0].Origin, ray[0].Direction);

	ray[0].TMin = ray[1].TMin = 0.00001;
	ray[0].TMax = ray[1].TMax = RAY_T_MAX;

	outputTexture[launchIndex] = float4(0,0,0,0);
	for (int r = 0; r < 1; r++) {
		RayPayload payload;
		payload.recursionDepth = 0;
		payload.hitT = 0;
#ifdef TRACE_NON_OPAQUE_SEPARATELY
		TraceRay(gAS, RAY_FLAG_CULL_NON_OPAQUE, 0xFF, 0, N_RAY_TYPES, 0, ray[r], payload);

		RayPayload payload_non_opaque;
		payload_non_opaque.recursionDepth = 0;
		payload_non_opaque.hitT = 0;

		ray.TMax = payload.hitT;
		TraceRay(gAS, RAY_FLAG_CULL_OPAQUE, 0xFF, 0, N_RAY_TYPES, 0, ray[r], payload_non_opaque);

		if (payload_non_opaque.hitT < RAY_T_MAX) {
			outputTexture[launchIndex] += payload_non_opaque.color;
		}
		else {
			outputTexture[launchIndex] += payload.color;
		}
#else // TRACE_NON_OPAQUE_SEPARATELY
		TraceRay(gAS, 0, 0xFF, 0, N_RAY_TYPES, 0, ray[r], payload);
#ifdef RAY_GEN_ALPHA_TEST
		uint i = 1;
		while (payload.color.a < 0.1) {
			i++;
			ray.TMin = payload.hitT + 0.01;
			payload.recursionDepth = 0;
			TraceRay(gAS, 0, 0xFF, 0, N_RAY_TYPES, 0, ray[r], payload);
		}
		payload.recursionDepth = i;
#endif // RAY_GEN_ALPHA_TEST

#ifdef DEBUG_RECURSION_DEPTH
		float t = payload.recursionDepth / 15.f;
		outputTexture[launchIndex] += float4(t, t, t, 1);
#elif defined(DEBUG_DEPTH)
#ifndef DEBUG_DEPTH_EXP
#define DEBUG_DEPTH_EXP 100
#endif

		float t = 1 - pow(1 - (payload.hitT / RAY_T_MAX), DEBUG_DEPTH_EXP);
		outputTexture[launchIndex] += float4(t, t, t, 1);
#else
		outputTexture[launchIndex] += payload.color;
#endif
#endif // !TRACE_NON_OPAQUE_SEPARATELY

	}

	//outputTexture[launchIndex] = saturate(outputTexture[launchIndex]);
}

[shader("closesthit")]
void closestHitTriangle(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	payload.hitT += RayTCurrent();
	//payload.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	//return;

	payload.recursionDepth++;
	if (payload.recursionDepth >= MAX_RAY_RECURSION_DEPTH)
	{
		payload.color = float4(1, 0, 0, 1);
		return;
	}

	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	float3 lightDir = -normalize(HitWorldPosition() - CB_SceneData.pLight.position);
	float lightDist = length(HitWorldPosition() - CB_SceneData.pLight.position);

	uint primitiveID = PrimitiveIndex();
	uint verticesPerPrimitive = 3;
	uint i1 = primitiveID * verticesPerPrimitive;
	uint i2 = primitiveID * verticesPerPrimitive + 1;
	uint i3 = primitiveID * verticesPerPrimitive + 2;

	//===Calculate UV Coordinates===
	float2 uv = barrypolation(barycentrics, vertices_uv[i1], vertices_uv[i2], vertices_uv[i3]);
	uv.y = -uv.y;

	//uv *= 8;
	//===Calculate Normal===
	float3 normalInLocalSpace = barrypolation(barycentrics, vertices_normal[i1], vertices_normal[i2], vertices_normal[i3]);
	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), float4(normalInLocalSpace, 0.f)));

#ifndef NO_NORMAL_MAP
	//===Add Normal Map===
	float3 tangent = mul(ObjectToWorld3x4(), barrypolation(barycentrics, vertices_tan_bi[i1].tangent, vertices_tan_bi[i2].tangent, vertices_tan_bi[i3].tangent));
	float3 binormal = mul(ObjectToWorld3x4(), barrypolation(barycentrics, vertices_tan_bi[i1].binormal, vertices_tan_bi[i2].binormal, vertices_tan_bi[i3].binormal));

	tangent = normalize(tangent);
	binormal = normalize(binormal);

	float4 bumpMapColor = sys_texNormMap.SampleLevel(samp, uv, 0);
	//bumpMapColor *= bumpMapColor.a;
	float3x3 tbn = float3x3(
		tangent,
		binormal,
		normalInWorldSpace
		);

	normalInWorldSpace = mul(normalize(bumpMapColor.xyz * 2.f - 1.f), tbn);
#endif //NO_NORMAL_MAP

	float lightMul = 1;

#ifndef NO_SHADOWS
	//Shadow
	RayDesc shadowRay;
	float t = dot(-WorldRayDirection(), normalInWorldSpace);
	shadowRay.Origin = HitWorldPosition() + normalInWorldSpace * 0.001f;
	//if (t >= 0) {
	//	shadowRay.Origin = HitWorldPosition() + normalInWorldSpace * 0.001f;
	//}
	//else {
	//	shadowRay.Origin = HitWorldPosition() - normalInWorldSpace * 0.001f;
	//}
	//payload.color = float4((t + 1) * 0.5, 0, 0, 1.0f);
	//return;

	shadowRay.Direction = lightDir;
	shadowRay.TMin = 0.00001;
	shadowRay.TMax = lightDist;

	RayPayload_shadow shadowPayload;
	shadowPayload.inShadow = 1;
	TraceRay(gAS, g_SHADOW_RAY_FLAGS, 0xFF, 1, N_RAY_TYPES, 1, shadowRay, shadowPayload);

	if (shadowPayload.inShadow)
	{
		lightMul = 0.1;
	}
#endif //NO_SHADOWS

	float4 albedo = sys_texAlbedo.SampleLevel(samp, uv, 0) * lightMul;
#ifndef NO_SHADING
	payload.color = float4(albedo.xyz * saturate(dot(lightDir, normalInWorldSpace)), 1.0f);
#else
	payload.color = float4(albedo.xyz, 1.0f);
#endif //NO_LIGHT

}

[shader("closesthit")]
void closestHitAlphaTest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	payload.hitT += RayTCurrent();
	payload.recursionDepth++;
	if (payload.recursionDepth >= MAX_RAY_RECURSION_DEPTH)
	{
		payload.color = float4(1, 0, 0, 1);
		return;
	}

	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	float3 lightDir = -normalize(HitWorldPosition() - CB_SceneData.pLight.position);
	float lightDist = length(HitWorldPosition() - CB_SceneData.pLight.position);

	uint primitiveID = PrimitiveIndex();
	uint verticesPerPrimitive = 3;
	uint i1 = primitiveID * verticesPerPrimitive;
	uint i2 = primitiveID * verticesPerPrimitive + 1;
	uint i3 = primitiveID * verticesPerPrimitive + 2;

	//===Calculate UV Coordinates===
	float2 uv = barrypolation(barycentrics, vertices_uv[i1], vertices_uv[i2], vertices_uv[i3]);
	uv.y = -uv.y;


	//uv *= 8;
	//===Calculate Normal===
	float3 normalInLocalSpace = barrypolation(barycentrics, vertices_normal[i1], vertices_normal[i2], vertices_normal[i3]);
	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), float4(normalInLocalSpace, 0.f)));
#ifdef CLOSEST_HIT_ALPHA_TEST
	float4 test = sys_texNormMap.SampleLevel(samp, uv, 0);
	if (test.a < 0.5f)
	{
		RayDesc ray;
		ray.Direction = WorldRayDirection();
		ray.Origin = HitWorldPosition() + ray.Direction * 0.15;
		ray.TMin = 0.00001;
		ray.TMax = RAY_T_MAX - payload.hitT;
		TraceRay(gAS, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);

		return;
	}
#endif

	float4 bumpMapColor = sys_texNormMap.SampleLevel(samp, uv, 0);
#ifndef NO_NORMAL_MAP
	//===Add Normal Map===
	float3 tangent = mul(ObjectToWorld3x4(), barrypolation(barycentrics, vertices_tan_bi[i1].tangent, vertices_tan_bi[i2].tangent, vertices_tan_bi[i3].tangent));
	float3 binormal = mul(ObjectToWorld3x4(), barrypolation(barycentrics, vertices_tan_bi[i1].binormal, vertices_tan_bi[i2].binormal, vertices_tan_bi[i3].binormal));

	tangent = normalize(tangent);
	binormal = normalize(binormal);

	//bumpMapColor *= bumpMapColor.a;
	float3x3 tbn = float3x3(
		tangent,
		binormal,
		normalInWorldSpace
		);

	normalInWorldSpace = mul(normalize(bumpMapColor.xyz * 2.f - 1.f), tbn);
#endif //NO_NORMAL_MAP

	float lightMul = 1;

#ifndef NO_SHADOWS
	//Shadow
	RayDesc shadowRay;
	float t = dot(-WorldRayDirection(), normalInWorldSpace);
	shadowRay.Origin = HitWorldPosition() + normalInWorldSpace * 0.001f;
	//if (t >= 0) {
	//	shadowRay.Origin = HitWorldPosition() + normalInWorldSpace * 0.001f;
	//}
	//else {
	//	shadowRay.Origin = HitWorldPosition() - normalInWorldSpace * 0.001f;
	//}
	//payload.color = float4((t + 1) * 0.5, 0, 0, 1.0f);
	//return;

	shadowRay.Direction = lightDir;
	shadowRay.TMin = 0.00001;
	shadowRay.TMax = lightDist;

	RayPayload_shadow shadowPayload;
	shadowPayload.inShadow = 1;
	TraceRay(gAS, g_SHADOW_RAY_FLAGS, 0xFF, 1, N_RAY_TYPES, 1, shadowRay, shadowPayload);

	if (shadowPayload.inShadow)
	{
		lightMul = 0.1;
	}
#endif //NO_SHADOWS

	float4 albedo = sys_texAlbedo.SampleLevel(samp, uv, 0) * lightMul;
#ifndef NO_SHADING
	payload.color = float4(albedo.xyz * saturate(dot(lightDir, normalInWorldSpace)), bumpMapColor.a);
#else
	payload.color = float4(albedo.xyz, bumpMapColor.a);
#endif //NO_LIGHT

}

[shader("anyhit")]
void anyHitAlphaTest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint primitiveID = PrimitiveIndex();
	uint verticesPerPrimitive = 3;
	uint i1 = primitiveID * verticesPerPrimitive;
	uint i2 = primitiveID * verticesPerPrimitive + 1;
	uint i3 = primitiveID * verticesPerPrimitive + 2;
	//===Calculate UV Coordinates===
	float2 uv = barrypolation(barycentrics, vertices_uv[i1], vertices_uv[i2], vertices_uv[i3]);
	uv.y = -uv.y;

	float4 normal = sys_texNormMap.SampleLevel(samp, uv, 0);
	if (normal.a < 0.5f)
	{
		IgnoreHit();
	}
}

[shader("miss")]
void miss(inout RayPayload payload) {
	payload.color = float4(0.3f, 0.3f, 0.9f, 1.0f);
	payload.hitT = RAY_T_MAX;
}

[shader("miss")]
void shadow_GeometryMiss(inout RayPayload_shadow payload)
{
	payload.inShadow = 0;
}

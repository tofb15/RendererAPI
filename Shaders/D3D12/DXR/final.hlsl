#define HLSL
#include "Common_hlsl_cpp.hlsli"

static const uint g_SHADOW_RAY_FLAGS = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
static const float RAY_T_MAX = 2000.0f;

//#define ONE_CHANNEL
#ifndef N_ALPHA_MAPS
#define N_ALPHA_MAPS 1
#endif

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

Texture2D<float4> sys_texAlbedo : register(t2, space0);
#ifdef ONE_CHANNEL
Texture2D<float1> sys_texAlphaMap[] : register(t3, space0);
#else
Texture2D<float4> sys_texAlphaMap[] : register(t3, space0);
#endif
// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void generateCameraRay(uint2 index, out float3 origin, out float3 direction) {
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

//#define DEBUG_RECURSION_DEPTH

[shader("raygeneration")]
void rayGen() {
	//return;
	uint2 launchIndex = DispatchRaysIndex().xy;

	RayDesc ray;

	generateCameraRay(launchIndex, ray.Origin, ray.Direction);

	ray.TMin = 0.00001;
	ray.TMax = RAY_T_MAX;

	RayPayload payload;
	payload.recursionDepth = 0;
	payload.hitT = 0;

	TraceRay(gAS, 0, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);

	outputTexture[launchIndex] = payload.color;

#ifdef DEBUG_RECURSION_DEPTH
	float max = 0;
	float t1 = saturate(((float)payload.recursionDepth - max) / (15.f - max));
	outputTexture[launchIndex] = float4(t1, t1, t1, 1);
#endif
}

[shader("closesthit")]
void closestHitTriangle(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
#ifdef SIMPLE_HIT
	payload.color = float4(0, 1, 0, 1);
	return;
#endif

	payload.hitT += RayTCurrent();
	payload.recursionDepth++;
	if (payload.recursionDepth >= MAX_RAY_RECURSION_DEPTH) {
		payload.color = float4(1, 1, 1, 1);
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

	//===Calculate Normal===
	float3 normalInLocalSpace = barrypolation(barycentrics, vertices_normal[i1], vertices_normal[i2], vertices_normal[i3]);
	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), float4(normalInLocalSpace, 0.f)));

	//float4 bumpMapColor = sys_texNormMap.SampleLevel(samp, uv, 0);
	float lightNormalAngle = saturate(dot(lightDir, normalInWorldSpace));
	float4 albedo = sys_texAlbedo.SampleLevel(samp, uv, 0);

#ifndef NO_SHADOWS
	float lightMul = 1;

	if (lightNormalAngle > 0) {
		//Shadow
		RayDesc shadowRay;
		shadowRay.Origin = HitWorldPosition() + normalInWorldSpace * 0.001f;

		shadowRay.Direction = lightDir;
		shadowRay.TMin = 0.00001;
		shadowRay.TMax = lightDist;

		RayPayload_shadow shadowPayload;
		shadowPayload.inShadow = 1;
		TraceRay(gAS, g_SHADOW_RAY_FLAGS, 0xFF, 1, N_RAY_TYPES, 1, shadowRay, shadowPayload);

		if (shadowPayload.inShadow) {
			lightMul = 0.2;
		}
	} else {

	}
	albedo *= lightMul;
#endif //NO_SHADOWS

#ifndef NO_SHADING
	albedo *= lightNormalAngle;
#endif //NO_LIGHT

	payload.color = float4(albedo.xyz, 1);

}

[shader("closesthit")]
void closestHitAlphaTest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
#ifdef SIMPLE_HIT
	payload.color = float4(1, 0, 0, 1);
	return;
#endif

	payload.hitT += RayTCurrent();
	payload.recursionDepth++;
	if (payload.recursionDepth >= MAX_RAY_RECURSION_DEPTH) {
		payload.color = float4(1, 1, 0, 1);
		return;
	}

	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);

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
	//float4 bumpMapColor = sys_texNormMap.SampleLevel(samp, uv, 0);

	float lightMul = 1;
	float3 lightDir = -normalize(HitWorldPosition() - CB_SceneData.pLight.position);

#ifndef NO_SHADOWS
	//Shadow
	float lightDist = length(HitWorldPosition() - CB_SceneData.pLight.position);
	RayDesc shadowRay;
	float t = dot(-WorldRayDirection(), normalInWorldSpace);
	
	shadowRay.Origin = HitWorldPosition() + normalInWorldSpace * 0.001f;
	shadowRay.Direction = lightDir;
	shadowRay.TMin = 0.00001;
	shadowRay.TMax = lightDist;

	RayPayload_shadow shadowPayload;
	shadowPayload.inShadow = 1;
	TraceRay(gAS, g_SHADOW_RAY_FLAGS, 0xFF, 1, N_RAY_TYPES, 1, shadowRay, shadowPayload);

	if (shadowPayload.inShadow) {
		lightMul = 0.1;
	}

#endif //NO_SHADOWS

	float4 albedo = sys_texAlbedo.SampleLevel(samp, uv, 0);
	albedo.xyz *= lightMul;
#ifndef NO_SHADING
	payload.color = float4(albedo.xyz * saturate(dot(lightDir, normalInWorldSpace)), 1.0f);
#else
	payload.color = float4(albedo.xyz, 1.0f);  
#endif //NO_LIGHT
	 
}


[shader("miss")]
void miss(inout RayPayload payload) {
	payload.color = float4(0.3f, 0.3f, 0.9f, 1.0f);
	payload.hitT = RAY_T_MAX;
}

[shader("miss")]
void miss_empty(inout RayPayload payload) {

}

[shader("miss")]
void shadow_GeometryMiss(inout RayPayload_shadow payload) {
	payload.inShadow = 0;
}

[shader("anyhit")]
void anyHitAlphaTest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
#ifdef DEBUG_RECURSION_DEPTH
	payload.recursionDepth++;
#endif

	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint primitiveID = PrimitiveIndex();
	uint verticesPerPrimitive = 3;
	uint i1 = primitiveID * verticesPerPrimitive;
	uint i2 = primitiveID * verticesPerPrimitive + 1;
	uint i3 = primitiveID * verticesPerPrimitive + 2;
	//===Calculate UV Coordinates===
	float2 uv = barrypolation(barycentrics, vertices_uv[i1], vertices_uv[i2], vertices_uv[i3]);
	uv.y = -uv.y;
	
	float alpha = 0;
	float scaler = 1;
	float2 scaledUV = uv;

	for (int i = 0; i < N_ALPHA_MAPS; i++) {
#ifdef ONE_CHANNEL
		alpha += sys_texAlphaMap[i].SampleLevel(samp, scaledUV, 0) / scaler;
#else
		alpha += sys_texAlphaMap[i].SampleLevel(samp, scaledUV, 0).a / scaler;
#endif
		scaler *= -15;
		scaledUV *= 5;
	}

	if (alpha < 0.5f) {
		IgnoreHit();
	}
}
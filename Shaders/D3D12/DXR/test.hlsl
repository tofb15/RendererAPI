#define HLSL
#include "Common_hlsl_cpp.hlsli"

static const uint g_SHADOW_RAY_FLAGS = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
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
RaytracingAccelerationStructure gAS : register(t0, space0);
RaytracingAccelerationStructure gAS_alpha : register(t0, space1);

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

	RayDesc ray;

	//generateCameraRay2(launchIndex, ray[0].Origin, ray[0].Direction, ray[1].Origin, ray[1].Direction);
	generateCameraRay(launchIndex, ray.Origin, ray.Direction);

	ray.TMin = 0.00001;
	ray.TMax = RAY_T_MAX;

	//outputTexture[launchIndex] = float4(0,0,0,0);
	//for (int r = 0; r < 1; r++) {
		RayPayload payload;
		payload.recursionDepth = 0;
		payload.hitT = 0;
//#ifdef TRACE_NON_OPAQUE_SEPARATELY
//		TraceRay(gAS, RAY_FLAG_CULL_NON_OPAQUE, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);
//
//		RayPayload payload_non_opaque;
//		payload_non_opaque.recursionDepth = 0;
//		payload_non_opaque.hitT = 0;
//
//		ray.TMax = payload.hitT;
//		TraceRay(gAS, RAY_FLAG_CULL_OPAQUE, 0xFF, 0, N_RAY_TYPES, 0, ray, payload_non_opaque);
//
//		if (payload_non_opaque.hitT < RAY_T_MAX) {
//			outputTexture[launchIndex] = payload_non_opaque.color;
//		}
//		else {
//			outputTexture[launchIndex] = payload.color;
//		}
//#else // TRACE_NON_OPAQUE_SEPARATELY
#ifdef RAY_GEN_ALPHA_TEST
#ifdef TRACE_NON_OPAQUE_SEPARATELY
		TraceRay(gAS, 0, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);
#endif //TRACE_NON_OPAQUE_SEPARATELY
		uint i = 1;
		//float3 c = payload.color.xyz;
		do {
			i++;
			ray.TMin = payload.hitT + 0.001f;
			payload.hitT = 0.0f;
			payload.recursionDepth = 0;
#ifdef TRACE_NON_OPAQUE_SEPARATELY
			TraceRay(gAS_alpha, 0, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);
#else // TRACE_NON_OPAQUE_SEPARATELY
			TraceRay(gAS, 0, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);
#endif // TRACE_NON_OPAQUE_SEPARATELY
		} while (payload.color.a < 0.5);
		payload.recursionDepth = i;
#else // RAY_GEN_ALPHA_TEST
		TraceRay(gAS, 0, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);
#ifdef TRACE_NON_OPAQUE_SEPARATELY
		ray.TMax = payload.hitT;
		TraceRay(gAS_alpha, 0, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);
#endif // TRACE_NON_OPAQUE_SEPARATELY
#endif // RAY_GEN_ALPHA_TEST

#ifdef DEBUG_RECURSION_DEPTH
		float max = 0;
		float t1 = saturate(((float)payload.recursionDepth - max) / (15.f- max));
		//t1 = payload.recursionDepth > 20;
		outputTexture[launchIndex] = float4(t1, t1, t1, 1);
//#ifdef RAY_GEN_ALPHA_TEST
//		outputTexture[launchIndex] = float4(saturate(c), 1.0f);
//#endif

#elif defined(DEBUG_DEPTH)
#ifndef DEBUG_DEPTH_EXP
#define DEBUG_DEPTH_EXP 100
#endif

		float t2 = 1 - pow(1 - (payload.hitT / RAY_T_MAX), DEBUG_DEPTH_EXP);
		outputTexture[launchIndex] = float4(t2, t2, t2, 1);
#else
		outputTexture[launchIndex] = payload.color;
#endif
//#endif // !TRACE_NON_OPAQUE_SEPARATELY

		uint lc = WaveGetLaneCount();
		uint li = WaveGetLaneIndex();
		float t3 = li / (float)lc;
		//t3 = 1 - pow(1 - t3, 2);
		//float t3 = WaveIsFirstLane();
		//float t3 = WaveActiveAllTrue(true);
		//float t3 = WaveActiveAllTrue(true);
		//uint3 dim = DispatchRaysDimensions();
		//float t3 = WaveReadLaneFirst(launchIndex.x + dim.x * launchIndex.y) / (dim.x * dim.z);
		//outputTexture[launchIndex] = float4(t3, t3, t3, 1);
		//t3 = li % lc < 6;
		//outputTexture[launchIndex] *= float4(t3, t3, t3, 1);
	//}

	//outputTexture[launchIndex] = saturate(outputTexture[launchIndex]);
}

[shader("closesthit")]
void closestHitTriangle(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	
#ifdef SIMPLE_HIT
	payload.color = float4(1, 1, 1, 1);
	return;
#endif
	
	payload.hitT += RayTCurrent();
	//payload.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	//return;

	payload.recursionDepth++;
	if (payload.recursionDepth >= MAX_RAY_RECURSION_DEPTH)
	{
		payload.color = float4(1, 1, 0, 1);
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
	float lightNormalAngle = saturate(dot(lightDir, normalInWorldSpace));

	float4 albedo = sys_texAlbedo.SampleLevel(samp, uv, 0);

	//float t = dot(-WorldRayDirection(), normalInWorldSpace);
	//payload.color = float4(t,t,t,1);
	//return;
#ifndef NO_SHADOWS
	float lightMul = 1;

	//float t = lightNormalAngle;
	//payload.color = float4(t,t,t,1);
	//return;
	if (lightNormalAngle > 0) {
		//Shadow
		RayDesc shadowRay;
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
			lightMul = 0.2;
			//albedo = payload.color;
		}
	}
	else {

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
	if (payload.recursionDepth >= MAX_RAY_RECURSION_DEPTH)
	{
		payload.color = float4(1, 1, 0, 1);
		return;
	}
	//return;

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
	float4 bumpMapColor = sys_texNormMap.SampleLevel(samp, uv, 0);

	//float t_ = dot(-WorldRayDirection(), normalInWorldSpace);
	//payload.color = float4(t_, t_, t_, 1);
	//return;
#ifdef CLOSEST_HIT_ALPHA_TEST_1
	if (bumpMapColor.a < 0.5f)
	{
		
		RayDesc ray;
		ray.Direction = WorldRayDirection();
		ray.Origin    = WorldRayOrigin();
		//ray.TMin = 0.00001;
		ray.TMin = payload.hitT + 0.00001f;
		ray.TMax = RAY_T_MAX;// -payload.hitT;
		payload.hitT = 0.0f;
		TraceRay(gAS, 0, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);
		///////////////
		return;
	}
#endif

	float4 albedo = sys_texAlbedo.SampleLevel(samp, uv, 0);
	albedo.a = bumpMapColor.a;

#ifdef CLOSEST_HIT_ALPHA_TEST_2
	
	if (payload.recursionDepth == 1)
	{
		payload.color = albedo;

		RayDesc ray;
		ray.Direction = WorldRayDirection();
		ray.Origin    = WorldRayOrigin();
		//ray.TMin = 0.00001;
		//ray.TMin = payload.hitT + 0.00001f;
		//payload.hitT = 0.0f;
		//TraceRay(gAS, 0, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);
		///////////////
		uint i = 1;
		//float3 c = payload.color.xyz;
		while (payload.color.a < 0.5) {
			i++;
			ray.TMin = payload.hitT + 0.001f;
			ray.TMax = RAY_T_MAX;// -payload.hitT;
			payload.hitT = 0.0f;
			payload.recursionDepth = 1;
			TraceRay(gAS, 0, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);
			//albedo = payload.color;
			//c += (payload.color.xyz);
		}
		payload.recursionDepth = i;
		if (payload.recursionDepth > 1) {
			return;
		}
	}
	else {
		if (bumpMapColor.a < 0.5f)
		{
			payload.color = float4(0, 0, 0, 0);
			return;
		}
	}
#endif

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
	float3 lightDir = -normalize(HitWorldPosition() - CB_SceneData.pLight.position);

#ifndef NO_SHADOWS
	//Shadow
	float lightDist = length(HitWorldPosition() - CB_SceneData.pLight.position);
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
	albedo.xyz *= lightMul;
#ifndef NO_SHADING
	payload.color = float4(albedo.xyz * saturate(dot(lightDir, normalInWorldSpace)), bumpMapColor.a);
#else
	payload.color = float4(albedo.xyz, bumpMapColor.a);
#endif //NO_LIGHT

}


[shader("miss")]
void miss(inout RayPayload payload) {
	//float3 dir = WorldRayDirection();
	//if (dir.y < 0) {
	//	//ground
	//	payload.color = float4(0.6f, 0.39f, 0.10f, 1.0f);
	//}
	//else {
		//Sky
		payload.color = float4(0.3f, 0.3f, 0.9f, 1.0f);
	//}
	payload.hitT = RAY_T_MAX;
}

[shader("miss")]
void shadow_GeometryMiss(inout RayPayload_shadow payload)
{
	payload.inShadow = 0;
}

#ifndef DICE_ANYHIT

[shader("anyhit")]
void anyHitAlphaTest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
#if defined(DEBUG_RECURSION_DEPTH) && !defined(DEBUG_RECURSION_DEPTH_MISS_ONLY) && !defined(DEBUG_RECURSION_DEPTH_HIT_ONLY)
	payload.recursionDepth++;
#endif
	//payload.color = float4(1,0,0,1);
	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint primitiveID = PrimitiveIndex();
	uint verticesPerPrimitive = 3;
	uint i1 = primitiveID * verticesPerPrimitive;
	uint i2 = primitiveID * verticesPerPrimitive + 1;
	uint i3 = primitiveID * verticesPerPrimitive + 2;
	//===Calculate UV Coordinates===
	float2 uv = barrypolation(barycentrics, vertices_uv[i1], vertices_uv[i2], vertices_uv[i3]);
	uv.y = -uv.y;
	//if (RayTCurrent() > payload.color.a) {
	//	payload.color.a = RayTCurrent();
	//}
	//payload.recursionDepth = payload.color.a / 20;
	float4 normal = sys_texNormMap.SampleLevel(samp, uv, 0);
	if (normal.a < 0.5f)
	{
#if defined(DEBUG_RECURSION_DEPTH) && defined(DEBUG_RECURSION_DEPTH_MISS_ONLY)
		payload.recursionDepth++;
#endif
		IgnoreHit();
	}
#if defined(DEBUG_RECURSION_DEPTH) && defined(DEBUG_RECURSION_DEPTH_HIT_ONLY)
	else {
		payload.recursionDepth++;
	}
#endif
}
/***************************************************************************************/

#else

#define ddx(x) x
#define ddy(x) x
#define discard
#define Sample(sampler, uv, ...) SampleLevel(sampler, uv, 0)
#define SampleGrad(sampler, uv, ...) SampleLevel(sampler, uv, 0)
#define clip(x)


uint getInstanceId() { return InstanceID(); }
struct VsInput
{
	float2 texCoord0;
	float3 pos;
	half3 normal;
};


float3 transformToWorldSpace(float3 objectPos)
{
	return mul(objectPos, ObjectToWorld3x4());
}
float3 rotateToWorldSpace(float3 objectSpaceVector)
{
	return mul(objectSpaceVector, (float3x3)ObjectToWorld3x4());
}

struct VsOutput
{
	float3 worldPos;
	float2 texCoord0;
	float eyeSurfaceAngle;
};

struct IaOutput
{
	VsInput v[3];
};

IaOutput runInputAssembler(uint primitiveId, uint instanceId)
{
	IaOutput iaOutput;

	//const bool isHalf = getIsHalfIndexBuffer();
	for (uint edgeId = 0; edgeId < 3; ++edgeId)
	{
		//const uint vertexStreamStride_0 = 0;

		const uint vertexId = primitiveId * 3 + edgeId;
		//const uint vi = 0;
		// Stream 0
		//const uint unalignedBase0 = 0 * vi;
		//uint4 vertexBlock_s0[2];
		//vertexBlock_s0[0] = g_vertexStream_0[unalignedBase0 / 16 + 0];
		//vertexBlock_s0[1] = g_vertexStream_0[unalignedBase0 / 16 + 1];

		iaOutput.v[edgeId].texCoord0 = 0;
		iaOutput.v[edgeId].texCoord0.xy = vertices_uv[vertexId]; 	// texCoord0
		iaOutput.v[edgeId].texCoord0.y = 1 - iaOutput.v[edgeId].texCoord0.y;

		iaOutput.v[edgeId].pos = 0;
		iaOutput.v[edgeId].pos.xyz = vertices_pos[vertexId]; 		// pos
		iaOutput.v[edgeId].normal = 0;
		iaOutput.v[edgeId].normal.xyz = vertices_normal[vertexId]; 	// normal
	}

	return iaOutput;
}


VsOutput runVertexMain(VsInput inputs)
{
	VsOutput outputs;

	float3 pos = inputs.pos;
	float3 normal = inputs.normal;

	pos = transformToWorldSpace(pos);
	float3 worldPos = pos;

	outputs.worldPos = worldPos;

	outputs.texCoord0 = inputs.texCoord0.rg;
	outputs.eyeSurfaceAngle = dot(normalize(CB_SceneData.cameraPosition.xyz - worldPos), normalize(rotateToWorldSpace(inputs.normal)));

	return outputs;
}

VsOutput runVertexShader(IaOutput isOutput, float3 bc)
{
	VsOutput vsOutput0 = runVertexMain(isOutput.v[0]);
	VsOutput vsOutput1 = runVertexMain(isOutput.v[1]);
	VsOutput vsOutput2 = runVertexMain(isOutput.v[2]);

	VsOutput vsOutput;
	vsOutput.texCoord0 = bc.x * vsOutput0.texCoord0 + bc.y * vsOutput1.texCoord0 + bc.z * vsOutput2.texCoord0;
	vsOutput.eyeSurfaceAngle = bc.x* vsOutput0.eyeSurfaceAngle + bc.y * vsOutput1.eyeSurfaceAngle + bc.z * vsOutput2.eyeSurfaceAngle;

	return vsOutput;
}

struct PsOutput
{
	float4 outColor0;
	float4 outColor1;
	float4 outColor2;
	float4 outColor3;
	float opacity;
};
PsOutput runPixelShader(VsOutput inputs)
{
	uint instanceId = getInstanceId();
	float Node34_output = (1.0f + 0.12);
	half4 TexBaseColor_output = (sys_texNormMap.Sample(samp, frac(float4(float2(inputs.texCoord0.r, inputs.texCoord0.g), 0., 0.).rg)));
	float Node33_output = saturate(Node34_output);

	half Node26_output = abs(inputs.eyeSurfaceAngle);
	half Node25_output = (1.0 - (Node26_output));
	half Node30_output = (Node25_output * Node25_output);
	half Node31_output = (Node30_output * Node25_output);

	half Node28_output = (1.0 - (Node31_output));
	float Node35_output = (TexBaseColor_output.a * Node33_output);
	float Node29_output = (Node28_output * Node35_output);

	half opacity = saturate(Node29_output);
	const half _opacity = opacity - 0.5;

	PsOutput psOutput;
	psOutput.opacity = _opacity;
	return psOutput;
}

void finalizePayload(inout RayPayload payload, float3 barycentrics, PsOutput psOutput)
{
	if (psOutput.opacity < 0)
	{
		IgnoreHit();
	}
}

struct Attributes { float2 barycentrics; };
[shader("anyhit")]
void anyHitAlphaTest(inout RayPayload payload, in Attributes attributes)
{
	const float3 barycentrics = float3(1.0 - attributes.barycentrics.x - attributes.barycentrics.y, attributes.barycentrics.x, attributes.barycentrics.y);

	// 'Input Assembler' stage
	IaOutput iaOutput = runInputAssembler(PrimitiveIndex(), getInstanceId());

	// 'Vertex shader' stage
	VsOutput vsOutput = runVertexShader(iaOutput, barycentrics);

	// 'Pixel shader' stage
	PsOutput psOutput = runPixelShader(vsOutput);

	// Pack ps output and write to payloadPacked
	finalizePayload(payload, barycentrics, psOutput);
}

#endif
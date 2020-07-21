#define HLSL
#include "..\..\Shaders\D3D12\DXR\Common_hlsl_cpp.hlsli"

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

	float3 worldHitPos = HitWorldPosition();
	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	float3 lightDir = -normalize(worldHitPos - CB_SceneData.pLight[0].position);
	float lightDist = length(worldHitPos - CB_SceneData.pLight[0].position);

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
	float3 lightDir = -normalize(HitWorldPosition() - CB_SceneData.pLight[0].position);

#ifndef NO_SHADOWS
	//Shadow
	float lightDist = length(HitWorldPosition() - CB_SceneData.pLight[0].position);
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
	
	float alpha = sys_texAlphaMap[0].SampleLevel(samp, uv, 0).a;

	if (alpha < 0.5f) {
		IgnoreHit();
	}
}
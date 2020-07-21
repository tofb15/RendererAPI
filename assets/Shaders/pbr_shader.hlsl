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

	float3 worldHitPoint = HitWorldPosition();
	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);

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
	float3 albedo = sys_texAlbedo.SampleLevel(samp, uv, 0).xyz;
	float lightMul = 1;

	float3 toCamera = normalize(CB_SceneData.cameraPosition - worldHitPoint);
	if(CB_SceneData.nLights > 0){
		float lightNormalAngle = 0;
		float3 lightDir = float3(1.0f,0.0f,0.0f);
		float lightDist = 0;
		float3 sum = float3(0,0,0);
		float dw = 1.0f/max(CB_SceneData.nLights, 1);
		for(int i = 0; i < CB_SceneData.nLights; i++){
			lightDir = normalize(CB_SceneData.pLight[i].position - worldHitPoint);
			lightDist = length(worldHitPoint - CB_SceneData.pLight[i].position);
			float lightRadius = CB_SceneData.pLight[i].reachRadius;
			if(lightDist > lightRadius){
				continue;
			}
			lightNormalAngle = dot(lightDir, normalInWorldSpace);
			if (lightNormalAngle > 0) {	
				//Check if the light is ocluded.
				RayDesc shadowRay;
				shadowRay.Origin = worldHitPoint + normalInWorldSpace * 0.001f;

				shadowRay.Direction = lightDir;
				shadowRay.TMin = 0.00001;
				shadowRay.TMax = lightDist;

				RayPayload_shadow shadowPayload;
				shadowPayload.inShadow = 1;
				TraceRay(gAS, g_SHADOW_RAY_FLAGS, 0xFF, 1, N_RAY_TYPES, 1, shadowRay, shadowPayload);

				float3 color = CB_SceneData.pLight[i].color;
    			float attenuation = pow(saturate(1.f - pow(lightDist/lightRadius, 2.f)), 2.f);
    			//attenuation *= 8.f;
    			float3 radiance   = color * attenuation;

				//If the light is not ocluded, Do light things.
				if (!shadowPayload.inShadow) {
					//Do light things
					sum += radiance * lightNormalAngle * dw;
				}
			}
		}
		sum = clamp(sum,0.01,1);
		albedo *= sum;
	}else{
		albedo *= 0.01f;
	}

	payload.color = float4(albedo, 1);

}
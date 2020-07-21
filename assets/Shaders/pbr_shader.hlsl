#define HLSL
#include "..\..\Shaders\D3D12\DXR\Common_hlsl_cpp.hlsli"

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}


inline float3 PBRLightContribution(in PointLight pl, in float dist, in float dw, in float3 N, in float3 L, in float NdotL, in float3 V, in float3 albedo, in float metallic, in float roughness){
    float3 H = normalize(V + L);
	float lightRadius = pl.reachRadius;
		
    float attenuation = pow(saturate(1.f - pow(dist/pl.reachRadius, 2.f)), 2.f);
    //float attenuation = pl.reachRadius / (dist*dist);
	//attenuation *= 8.f;

    float3 radiance = pl.color * attenuation;
	
	float3 F  = fresnelSchlick(max(dot(V,H), 0.0), lerp(0.04, albedo, metallic));
	float NDF = DistributionGGX(N, H, roughness);       
	float G   = GeometrySmith(N, V, L, roughness);  
	float3 numerator  = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	float3 specular   = numerator / max(denominator, 0.001);  
	float3 kS = F;
	float3 kD = float3(1.0,1.0,1.0) - kS;
	kD *= 1.0 - metallic;
    return (kD * albedo / PI + specular) * radiance * NdotL;
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

	float3 albedo    = sys_textures[0].SampleLevel(samp, uv, 0).xyz;
	float4 bumpMapColor = sys_textures[NORMAL_TEX_POS].SampleLevel(samp, uv, 0);
	float metal     = 0.1;//sys_textures[METAL_TEX_POS].SampleLevel(samp, uv, 0).x;
	float roughness = sys_textures[ROUGHNESS_TEX_POS].SampleLevel(samp, uv, 0).x * 1.8;

	roughness = saturate(roughness);

	//===Calculate Normal===
	float3 normalInLocalSpace = barrypolation(barycentrics, vertices_normal[i1], vertices_normal[i2], vertices_normal[i3]);
	float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), float4(normalInLocalSpace, 0.f)));

//#define NO_NORMAL_MAP
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

	
	float3 Lo = float3(0,0,0);

	if(CB_SceneData.nLights > 0){
		float lightNormalAngle = 0;
		float3 toLightDir = float3(1.0f,1.0f,0.0f);
		float3 toCameraDir = normalize(CB_SceneData.cameraPosition - worldHitPoint);
		float LightDistToHit = 0;

		for(int i = 0; i < CB_SceneData.nLights; i++){
			toLightDir = normalize(CB_SceneData.pLight[i].position - worldHitPoint);
			lightNormalAngle = max(dot(toLightDir, normalInWorldSpace), 0);
			if (lightNormalAngle > 0) {	
				float LightDistToHit = length(worldHitPoint - CB_SceneData.pLight[i].position);
				if(LightDistToHit > CB_SceneData.pLight[i].reachRadius){
					continue;
				}
				//Check if the light is ocluded.
				RayDesc shadowRay;
				shadowRay.Origin = worldHitPoint + normalInWorldSpace * 0.001f;
				shadowRay.Direction = toLightDir;
				shadowRay.TMin = 0.00001;
				shadowRay.TMax = LightDistToHit;
				RayPayload_shadow shadowPayload;
				shadowPayload.inShadow = 1;
				TraceRay(gAS, g_SHADOW_RAY_FLAGS, 0xFF, 1, N_RAY_TYPES, 1, shadowRay, shadowPayload);
				//If the light is not ocluded, Do light things.
				if (!shadowPayload.inShadow) {
					//Do light things
					float dw = 1.0f/max(CB_SceneData.nLights, 1);
					Lo += PBRLightContribution(CB_SceneData.pLight[i], LightDistToHit, dw,  normalInWorldSpace, toLightDir, lightNormalAngle, toCameraDir, albedo, metal, roughness);

				}
			}
		}
	}

	Lo = saturate(Lo);
	payload.color = float4(Lo, 1);
}
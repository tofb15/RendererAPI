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


inline float3 PBRLightContribution(in PointLight pl, in float dist, in float3 N, in float3 L, in float NdotL, in float3 V, in float3 albedo, in float metallic, in float roughness){
    float3 H = normalize(V + L);
	float lightRadius = pl.reachRadius;
		
    //float attenuation = pow(saturate(1.f - pow(dist/pl.reachRadius, 2.f)), 2.f);
    //float attenuation = pow(saturate(1.f - pow(dist/pl.reachRadius, 2.f)), 4.f);
    float attenuation = lightRadius / (dist*dist + 1);
	//attenuation *= 8.f;
    //float attenuation = pow(saturate(1.f - pow(dist/lightRadius, 4.f)), 4.f) / (dist * dist + 1.f);
    //attenuation *= 8.f;

    float3 radiance = normalize(pl.color) * attenuation;
	
	float3 F  = fresnelSchlick(max(dot(H,V), 0.0), lerp(float3(0.04, 0.04, 0.04), albedo, metallic));
	float NDF = DistributionGGX(N, H, roughness);       
	float G   = GeometrySmith(N, V, L, roughness);  
	float3 numerator  = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	float3 specular   = numerator / max(denominator, 0.001);  
	float3 kS = F;
	float3 kD = float3(1.0,1.0,1.0) - kS;
	kD *= 1.0 - metallic;
    return (kD * albedo / PI + specular) * NdotL * radiance;
	//return float3(1,1,1) * attenuation;	
	//return float3(1,1,1) * dist/1000.0;
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
	float4 RAOM = sys_textures[ROUGHNESS_TEX_POS].SampleLevel(samp, uv, 0);
	float roughness = RAOM.r * 1;
	float ao 		= RAOM.g * 1;
	float metal     = RAOM.b * 1;

	//===Calculate Normal===
	float3 normalInLocalSpace = barrypolation(barycentrics, vertices_normal[i1], vertices_normal[i2], vertices_normal[i3]);
	//float3 normalInWorldSpace = normalize(mul(ObjectToWorld3x4(), float4(normalInLocalSpace, 0.f)));
	//float3 binormal = mul(ObjectToWorld3x4(), barrypolation(barycentrics, vertices_tan_bi[i1].binormal, vertices_tan_bi[i2].binormal, vertices_tan_bi[i3].binormal));
	//float3 tangent = mul(ObjectToWorld3x4(), barrypolation(barycentrics, vertices_tan_bi[i1].tangent, vertices_tan_bi[i2].tangent, vertices_tan_bi[i3].tangent));

	float3 normalInWorldSpace = normalize(mul(float4(normalInLocalSpace, 0.f),ObjectToWorld4x3()));
	float3 normalInWorldSpace_noNormalmap = normalInWorldSpace;


//#define NO_NORMAL_MAP
#ifndef NO_NORMAL_MAP
	//===Add Normal Map===
	float3 binormal = mul(barrypolation(barycentrics, vertices_tan_bi[i1].binormal, vertices_tan_bi[i2].binormal, vertices_tan_bi[i3].binormal),ObjectToWorld4x3());
	float3 tangent = mul(barrypolation(barycentrics, vertices_tan_bi[i1].tangent, vertices_tan_bi[i2].tangent, vertices_tan_bi[i3].tangent),ObjectToWorld4x3());
	tangent = normalize(tangent);
	binormal = normalize(binormal);

    bumpMapColor = (bumpMapColor * 2.0f) - 1.0f;
    normalInWorldSpace = (bumpMapColor.x * tangent) + (bumpMapColor.y * binormal) + (bumpMapColor.z * normalInWorldSpace);
    normalInWorldSpace = normalize(normalInWorldSpace);

	//normalInWorldSpace = mul(normalize(bumpMapColor.xyz * 2.f - 1.f), tbn);
#endif //NO_NORMAL_MAP

	
	float3 Lo = float3(0,0,0);

	float3 toCameraDir = normalize(CB_SceneData.cameraPosition - worldHitPoint);
	float CDotN = dot(normalInWorldSpace_noNormalmap, toCameraDir);
	if(CDotN <= 0){
		normalInWorldSpace *= -1;
		normalInWorldSpace_noNormalmap *= -1;
	}
	
	if(CB_SceneData.nLights > 0){
		float lightNormalAngle = 0;
		float3 toLightDir = float3(1.0f,1.0f,0.0f);
		float LightDistToHit = 0;
		float dw = 1.0f/CB_SceneData.nLights;

		for(int i = 0; i < CB_SceneData.nLights; i++){
			if(all(CB_SceneData.pLight[i].color == 0.0)){
				continue;
			}
			toLightDir = normalize(CB_SceneData.pLight[i].position - worldHitPoint);	
			lightNormalAngle = max(dot(normalInWorldSpace,toLightDir), 0);	
			if (lightNormalAngle > 0) {	
				LightDistToHit = length(worldHitPoint - CB_SceneData.pLight[i].position);
				if(LightDistToHit > CB_SceneData.pLight[i].reachRadius){
					//continue;
				}

				//If the light is not ocluded, Do light things.
				if (!PointInShadow(worldHitPoint + normalInWorldSpace_noNormalmap * 0.0001f, toLightDir, LightDistToHit)) {
					//Do light things
					Lo += 1 * PBRLightContribution(CB_SceneData.pLight[i], LightDistToHit,  normalInWorldSpace, toLightDir, lightNormalAngle, toCameraDir, albedo, metal, roughness);
				}
			}
		}
	}

	float3 ambient = 0.08 * albedo * ao;
	Lo = saturate(Lo + ambient);
	//payload.color = float4(normalInWorldSpace, 1);
	payload.color = float4(CDotN,CDotN,CDotN, 1);
	payload.color = abs(float4(CDotN,CDotN,CDotN, 1));
	payload.color = float4(Lo, 1);
	
	//payload.color = float4(tangent, 1);
	//payload.color = float4(abs(normalInLocalSpace), 1);
	//payload.color = float4(worldHitPoint/100, 1);
}
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
    TraceRay(gAS, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, N_RAY_TYPES, 0, ray, payload);
    //float3 camPos = CB_SceneData.cameraPosition;
	////outputTexture[launchIndex] = float4(launchIndex.x / 64.0f, launchIndex.y / 64.0f, 0.0f, 1.0f);
    
    outputTexture[launchIndex] = payload.color; //float4(camPos.x, camPos.y, camPos.z, 0);
    //outputTexture[launchIndex] = float4(ray.Direction.x, ray.Direction.y, ray.Direction.z, 1.0f);
    //ray.Origin /= 1000;
    //outputTexture[launchIndex] = float4(ray.Origin.x, ray.Origin.y, ray.Origin.z, 1.0f);
    //float2 screenPos = (float2)launchIndex / DispatchRaysDimensions().xy;
    //outputTexture[launchIndex] = float4(screenPos, 0, 1.0f);

}

[shader("closesthit")]
void closestHitTriangle(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
    
    //payload.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    //return;
    
    payload.recursionDepth++;
    if (payload.recursionDepth > 2)
    {
        return;
    }
     
    float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
    float3 lightDir = -normalize(HitWorldPosition() - CB_SceneData.pLight.position);
    float3 lightDist = length(HitWorldPosition() - CB_SceneData.pLight.position);

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
    
    //bumpMapColor = bumpMapColor * 2.0f - 1.0f; // Change interval from (0, 1) to (-1, 1)
    //normalInWorldSpace = bumpMapColor.x * tangent + bumpMapColor.y * binormal + bumpMapColor.z * normalInWorldSpace.xyz;
    //normalInWorldSpace = normalize(normalInWorldSpace);
    
    //Shadow
    RayDesc shadowRay;
    shadowRay.Origin = HitWorldPosition() + normalInWorldSpace * 0.01f;
    shadowRay.Direction = lightDir;
    shadowRay.TMin = 0.00001;
    shadowRay.TMax = lightDist;
    
    RayPayload_shadow shadowPayload;
    shadowPayload.inShadow = 1;  
    TraceRay(gAS, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 1, N_RAY_TYPES, 1, shadowRay, shadowPayload);

    float lightMul = 1;
    
    if (shadowPayload.inShadow)
    {
        lightMul = 0.1;
        //payload.color = float4(0.1f, 0.1f, 0.1f, 0.1f);
        //return;
    }
  
    float4 albedo = sys_texAlbedo.SampleLevel(samp, uv, 0) * lightMul;
    
    //float t = pow((1000 - RayTCurrent()) / 1000.f, 4);
    //if (uv.x < 0)
    //{
    //    payload.color = float4(1.0f, 1.0f, 1.0f, 1.0f);      
    //}
    //else
    //{
    //    payload.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    //}
    //payload.color = float4(vertices_uv[i3], 0.0f, 1.0f);
    //payload.color = float4(uv, 0.0f, 1.0f);
    payload.color = float4(albedo.xyz * saturate(dot(lightDir, normalInWorldSpace)), 1.0f);
    
    //payload.color = float4(albedo.x, albedo.y, albedo.z, 1.0f);
    //payload.color = float4(WorldRayDirection().x, WorldRayDirection().y, WorldRayDirection().z, 1.0f);
    //payload.color = float4(WorldRayOrigin().x, WorldRayOrigin().y, WorldRayOrigin().z, 1.0f);
    //payload.color = float4(CB_SceneData.cameraPosition.x, CB_SceneData.cameraPosition.y, CB_SceneData.cameraPosition.z, 1.0f);
    //payload.color /= 1000;
}

[shader("anyhit")]
void anyHitTriangle(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
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
	payload.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
}

[shader("miss")]
void shadow_GeometryMiss(inout RayPayload_shadow payload)
{
    payload.inShadow = 0;
}

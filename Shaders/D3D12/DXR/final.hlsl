#define HLSL
#include "Common_hlsl_cpp.hlsli"

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

	TraceRay(gAS, 0, HIT_BY_PRIMARY_RAYS_FLAG, 0, N_RAY_TYPES, 0, ray, payload);

	outputTexture[launchIndex] = payload.color;

#ifdef DEBUG_RECURSION_DEPTH
	float max = 0;
	float t1 = saturate(((float)payload.recursionDepth - max) / (15.f - max));
	outputTexture[launchIndex] = float4(t1, t1, t1, 1);
#endif
}

[shader("miss")]
void miss(inout RayPayload payload) {
	payload.color = float4(0.00f, 0.00f, 0.0f, 1.0f);
	payload.hitT = RAY_T_MAX;
}

[shader("miss")]
void miss_empty(inout RayPayload payload) {

}

[shader("miss")]
void shadow_GeometryMiss(inout RayPayload_shadow payload) {
	payload.inShadow = 0;
}

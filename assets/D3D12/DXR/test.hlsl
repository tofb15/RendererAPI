struct RayPayload {
	float4 color;
	uint recursionDepth;
};

RaytracingAccelerationStructure gAS : register(t0);
RWTexture2D<float4> outputTexture : register(u1);
sampler samp : register(s0);

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
//inline void generateCameraRay(uint2 index, out float3 origin, out float3 direction) {
//	float2 xy = index + 0.5f; // center in the middle of the pixel.
//	float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;
//
//	// Invert Y for DirectX-style coordinates.
//	screenPos.y = -screenPos.y;
//
//	// Unproject the pixel coordinate into a ray.
//	float4 world = mul(CB_SceneData.projectionToWorld, float4(screenPos, 0, 1));
//
//	world.xyz /= world.w;
//	origin = CB_SceneData.cameraPosition.xyz;
//	direction = normalize(world.xyz - origin);
//}

[shader("raygeneration")]
void rayGen() {
	uint2 launchIndex = DispatchRaysIndex().xy;

	RayDesc ray;
	ray.Origin = float3(launchIndex.x, launchIndex.y, -10);
	ray.Direction = float3(0, 0, 1);
	ray.TMin = 0.00001;
	ray.TMax = 10000.0;

	RayPayload payload;
	TraceRay(gAS, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);

	//outputTexture[launchIndex] = float4(launchIndex.x / 64.0f, launchIndex.y / 64.0f, 0.0f, 1.0f);
	outputTexture[launchIndex] = payload.color;
}

[shader("closesthit")]
void closestHitTriangle(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	payload.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
}

[shader("miss")]
void miss(inout RayPayload payload) {
	payload.color = float4(0.0f, 0.0f, 1.0f, 1.0f);
}
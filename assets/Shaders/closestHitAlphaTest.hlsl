
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
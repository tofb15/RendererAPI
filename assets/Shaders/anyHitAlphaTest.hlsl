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
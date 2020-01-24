struct RayPayload {
	float4 color;
	uint recursionDepth;
};

[shader("raygeneration")]
void rayGen() {

}

[shader("closesthit")]
void closestHitTriangle(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {

}

[shader("miss")]
void miss(inout RayPayload payload) {

}
#define HLSL
#include "..\..\Shaders\D3D12\DXR\Common_hlsl_cpp.hlsli"

[shader("closesthit")]
void wireframeShadering_closesthit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	float a = 500;
    float t = a / (pow(RayTCurrent(), 2) + a);
    payload.color = float4(1, 1-t, 1-t, 1);
	return;
}

[shader("anyhit")]
void wireframeAnyHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
#ifdef DEBUG_RECURSION_DEPTH
	payload.recursionDepth++;
#endif

	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint primitiveID = PrimitiveIndex();
	uint verticesPerPrimitive = 3;
	uint i1 = primitiveID * verticesPerPrimitive;
	uint i2 = primitiveID * verticesPerPrimitive + 1;
	uint i3 = primitiveID * verticesPerPrimitive + 2;

	float3 l1 = vertices_pos[i2] - vertices_pos[i1];
    float3 l2 = vertices_pos[i3] - vertices_pos[i1];
    float3 l3 = vertices_pos[i2] - vertices_pos[i3];

    //Get the intersection point between the ray and the triangle in ObjectLocalSpace. We use ObjectLocalSpace since the triangle vericies is described in this coordinate system.
    float3 hitPos = HitObjectPosition();
    //Project the intersection point onto each edge of the triangle
    l1 = ProjPointToLine(hitPos, vertices_pos[i1], l1);
    l2 = ProjPointToLine(hitPos, vertices_pos[i1], l2);
    l3 = ProjPointToLine(hitPos, vertices_pos[i3], l3);
    //Calculate the distance from each projected point to the intersection point
    float3 lineDist = float3(length(l1-hitPos), length(l2-hitPos), length(l3-hitPos));
    //Find the clostest distance to any of the edges
	float minDist = min(lineDist.x, min(lineDist.y, lineDist.z));
    //Calculate the sum of each distance. This will be used to set the thickness of the wireframe line (larger triangles -> greater sum -> thicker line).
    float distSum = lineDist.x + lineDist.y + lineDist.z;
    //Calculate a distance threshold. If the minimum edge distance is creater than this threshold, the hit should be discarded. The Ray travel distance (RayTCurrent()) is weighted in aswell.
    float threshold;
    threshold = RayTCurrent() * distSum * 0.0008;
    threshold = clamp(threshold, 0.001, 0.3);

    //Ignore the hit if minDist is to large
	if (minDist > threshold) {
		IgnoreHit();
	}
}

[shader("anyhit")]
void dissolve_anyhit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
#ifdef DEBUG_RECURSION_DEPTH
	payload.recursionDepth++;
#endif
    float dissolveStartDist = 3;   
	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint primitiveID = PrimitiveIndex();
	uint verticesPerPrimitive = 3;
	uint i1 = primitiveID * verticesPerPrimitive;
	uint i2 = primitiveID * verticesPerPrimitive + 1;
	uint i3 = primitiveID * verticesPerPrimitive + 2;
	//===Calculate UV Coordinates===
	float3 l1 = vertices_pos[i2] - vertices_pos[i1];
    float3 l2 = vertices_pos[i3] - vertices_pos[i1];
    float3 l3 = vertices_pos[i2] - vertices_pos[i3];

    float3 hitPos = HitObjectPosition();
    l1 = ProjPointToLine(hitPos, vertices_pos[i1], l1);
    l2 = ProjPointToLine(hitPos, vertices_pos[i1], l2);
    l3 = ProjPointToLine(hitPos, vertices_pos[i3], l3);

    float3 lineDist = float3(length(l1-hitPos), length(l2-hitPos), length(l3-hitPos));
	float minDist = min(lineDist.x, min(lineDist.y, lineDist.z));

    float threshold;
    threshold = pow(RayTCurrent()*0.1 - dissolveStartDist, 2);
    threshold = clamp(threshold, RayTCurrent() * 0.0000, 1000);
 
	if (minDist > threshold) {
		IgnoreHit();
	}
}

[shader("anyhit")]
void dissolve_alphatested_anyhit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
#ifdef DEBUG_RECURSION_DEPTH
	payload.recursionDepth++;
#endif
    float dissolveStartDist = 4;   

	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	uint primitiveID = PrimitiveIndex();
	uint verticesPerPrimitive = 3;
	uint i1 = primitiveID * verticesPerPrimitive;
	uint i2 = primitiveID * verticesPerPrimitive + 1;
	uint i3 = primitiveID * verticesPerPrimitive + 2;
	//===Calculate UV Coordinates===
	float3 l1 = vertices_pos[i2] - vertices_pos[i1];
    float3 l2 = vertices_pos[i3] - vertices_pos[i1];
    float3 l3 = vertices_pos[i2] - vertices_pos[i3];

    float3 hitPos = HitObjectPosition();
    l1 = ProjPointToLine(hitPos, vertices_pos[i1], l1);
    l2 = ProjPointToLine(hitPos, vertices_pos[i1], l2);
    l3 = ProjPointToLine(hitPos, vertices_pos[i3], l3);

    float3 lineDist = float3(length(l1-hitPos), length(l2-hitPos), length(l3-hitPos));
	float minDist = min(lineDist.x, min(lineDist.y, lineDist.z));

    float threshold;

    threshold = pow(RayTCurrent()*0.1 - dissolveStartDist, 2);
    threshold = clamp(threshold, RayTCurrent() * 0.0000, 1000);

	if (minDist > threshold) {
		IgnoreHit();
	}else{
        float2 uv = barrypolation(barycentrics, vertices_uv[i1], vertices_uv[i2], vertices_uv[i3]);
	    uv.y = -uv.y;
	
	    float alpha = sys_textures[ALBEDO_TEX_POS].SampleLevel(samp, uv, 0).a;

	    if (alpha < 0.5f) {
	    	IgnoreHit();
	    }
    }
}
#define HLSL
#include "..\..\Shaders\D3D12\DXR\Common_hlsl_cpp.hlsli"

[shader("closesthit")]
void unlit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
	payload.color = float4(1, 1, 1, 1);
	return;
}
RWTexture2D<float4> uav1 : register(u0);
Texture2D g_texture : register(t0);

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{

	uav1[DTid.xy] = g_texture.Load(int3(DTid.xy, 0));//float4(1.0f, 1.0f, 1.0f, 1.0f);
}
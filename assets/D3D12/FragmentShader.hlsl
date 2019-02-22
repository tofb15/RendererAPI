Texture2D g_texture : register(t0);
sampler samp : register(s0);

struct VSOut
{
    float4 pos : SV_POSITION;
#ifdef NORMAL
	float4 normal	: NORM;
#endif
#ifdef TEXTCOORD
	float2 uv		: UV;
#endif
#ifdef DIFFUSE_TINT
	float4 color	: COL;
#endif
};


float4 main(VSOut input) : SV_TARGET0
{

#ifdef TEXTCOORD
	//float4 finalColor = float4(input.uv.x, input.uv.y, 0, 1);
	//float4 finalColor = float4(g_texture.Load(int3(60100, 0, 0)).xyz, 1);
	float4 finalColor = g_texture.Sample(samp, input.uv);
#elif defined(NORMAL)
    float4 finalColor = -1*float4(input.normal.xyz, 1.0f);
#else
    float4 finalColor = float4(1, 0, 1, 1);
#endif

	return finalColor;
}
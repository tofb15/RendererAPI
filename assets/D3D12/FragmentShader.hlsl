Texture2D g_texture[] : register(t0);

sampler samp : register(s0);

struct VSOut
{
    float4 pos : SV_POSITION;
	uint instanceID : INSTANCE;

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
	float4 finalColor = g_texture[input.instanceID].Sample(samp, input.uv);
	
	#ifdef NORMAL
		float3 lightDir = float3(-1.0f, 1.0f, -0.5f);
		
		finalColor = saturate(finalColor * dot(lightDir, input.normal.xyz)) + finalColor*0.1;
		finalColor.w = 1.0f;
	#endif
	
#elif defined(NORMAL)
    float4 finalColor = -1*float4(input.normal.xyz, 1.0f);
#else
    float4 finalColor = float4(1, 0, 1, 1);
#endif

	return finalColor;
}
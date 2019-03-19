Texture2D g_texture : register(t0);

struct VSOut
{
	float4 pos		: SV_POSITION;
};

float4 main(VSOut input) : SV_TARGET0
{
    //float4 finalColor = float4(1, 0, 1, 1);
	float4 finalColor = g_texture.Load(int3(10,10, 0));
	
	return finalColor;
}
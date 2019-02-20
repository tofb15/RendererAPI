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

#ifdef NORMAL
    float4 finalColor = -1*float4(input.normal.xyz, 1.0f);
#else
    float4 finalColor = float4(1, 1, 1, 1);
#endif

	return finalColor;
}
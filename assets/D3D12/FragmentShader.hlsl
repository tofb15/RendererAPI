struct VSOut
{
	float4 pos		: SV_POSITION;
};


float4 main(VSOut input) : SV_TARGET0
{
	float4 finalColor = float4(1, 1, 1, 1);

	return finalColor;
}
struct VSOut
{
	float4 pos		: SV_POSITION;
};

VSOut main(uint index : SV_VertexID)
{
	VSOut output = (VSOut)0;
 
	output.pos = float4((index % 2) * 4.0f - 1.0f, (index / 2) * 4.0f - 1.0f, 1.0f, 1.0f);

	return output;
}
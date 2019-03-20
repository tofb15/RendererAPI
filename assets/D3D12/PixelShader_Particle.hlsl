struct GSOut
{
    float4 pos : SV_POSITION;
};

float4 main(GSOut input) : SV_TARGET0
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
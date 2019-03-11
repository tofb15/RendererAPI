struct VSOut
{
	float4 pos		: SV_POSITION;
	float4 wPos		: WPOS;
	uint instanceID : INSTANCE;

#ifdef TEXTCOORD
	float2 uv		: UV;
#endif
	float4 color	: COL;
};

struct GSOut
{
    float4 pos : SV_POSITION;
	uint instanceID : INSTANCE;

	float4 normal	: NORM;
#ifdef TEXTCOORD
	float2 uv		: UV;
#endif
	float4 color	: COL;
};

[maxvertexcount(3)]
void main(triangle VSOut input[3], inout TriangleStream<GSOut> OutputStream)
{
    GSOut output[3];

    float3 v1 = input[0].wPos.xyz - input[1].wPos.xyz;
    float3 v2 = input[2].wPos.xyz - input[1].wPos.xyz;
    float4 normal = float4(normalize(cross(v2,v1)),0.0f);
	
	for (int i = 0; i < 3; i++)
	{
		output[i].pos = input[i].pos;
		output[i].color = input[i].color;
		output[i].instanceID = input[i].instanceID;
		output[i].normal = normal;
		#ifdef TEXTCOORD
		output[i].uv = input.uv;
		#endif
	}

	OutputStream.Append(output[0]);
	OutputStream.Append(output[1]);
	OutputStream.Append(output[2]);
	
}
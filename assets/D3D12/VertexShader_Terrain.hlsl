struct VSIn
{
	float3 pos		: POS;
#ifdef TEXTCOORD
	float2 uv		: UV;
#endif
};

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

cbuffer CB : register(b0)
{
    float4x4 viewPerspective;
	int matrixIndex;
}

StructuredBuffer<float4x4> sr : register(t1);


VSOut main(VSIn input, uint index : SV_VertexID, uint instanceID : SV_InstanceID)
{
	VSOut output = (VSOut)0;
 
	output.pos = mul(viewPerspective, mul(sr[matrixIndex + instanceID], float4(input.pos, 1.0f)));
	output.wPos = float4(input.pos, 1.0f);
	output.color = float4( index % 2, 0.0f, 0.0f, 1.0f);
	
	output.instanceID = instanceID;

#ifdef TEXTCOORD
	output.uv = input.uv;
#endif
#ifdef DIFFUSE_TINT
	output.color = float4(color.xyz, 1.0f);
#endif

	return output;
}
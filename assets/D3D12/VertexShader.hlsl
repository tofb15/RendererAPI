struct VSIn
{
	float3 pos		: POS;
#ifdef NORMAL
	float3 normal	: NORM;
#endif
#ifdef TEXTCOORD
	float2 uv		: UV;
#endif
};

struct VSOut
{
	float4 pos		: SV_POSITION;
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

	output.instanceID = instanceID;
#ifdef NORMAL
	output.normal = float4(input.normal, 0.0f);
#endif
#ifdef TEXTCOORD
	output.uv = input.uv;
#endif
#ifdef DIFFUSE_TINT
	output.color = float4(color.xyz, 1.0f);
#endif

	return output;
}
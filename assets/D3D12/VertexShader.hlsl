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
//cbuffer CBBig : register(b1)
//{
	//float4x4 worlds1[1024];
	//float4x4 worlds2[1024];
	//float4x4 worlds3[1024];
	//float4x4 worlds4[1024];
	//float4x4 worlds5[1024];
	//float4x4 worlds6[1024];
	//float4x4 worlds7[1024];
	//float4x4 worlds8[1024];
	//float4x4 worlds9[1024];
	//float4x4 worlds10[128];
//}

StructuredBuffer<float4x4> sr : register(t1);


VSOut main(VSIn input, uint index : SV_VertexID, uint instanceID : SV_InstanceID)
{
	VSOut output = (VSOut)0;
 
	// output.pos = mul(float4(input.pos, 1.0f), viewPerspective);
	output.pos = mul(mul(float4(input.pos, 1.0f), sr[matrixIndex + instanceID]), viewPerspective);
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
struct VSIn
{
	float3 pos		: POS;
#ifdef NORMAL
	float3 normal	: NORM;
#endif
#ifdef TEXTCOORD
	float2 uv		: UV;
#endif
#ifdef NMAP
	float3 tangent	: TAN;
	float3 binormal	: BI;
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
//#ifdef DIFFUSE_TINT
	float4 color	: COL;
//#endif
#ifdef NMAP
	float4 tangent	: TAN;
	float4 binormal	: BI;
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

	output.color = float4( index % 2, 0.0f, 0.0f, 1.0f);
	
	output.instanceID = instanceID;
#ifdef NORMAL
	float3 normal = mul(input.normal.xyz, (float3x3)sr[matrixIndex + instanceID]);
	normal = normalize(normal);

	output.normal = float4(normal, 0);
#endif
#ifdef TEXTCOORD
	output.uv = input.uv;
#endif
#ifdef DIFFUSE_TINT
	output.color = float4(color.xyz, 1.0f);
#endif
#ifdef NMAP
	/*float3 tangent = mul(input.tangent.xyz, (float3x3)sr[matrixIndex + instanceID]);
	float3 binormal = mul(input.binormal.xyz, (float3x3)sr[matrixIndex + instanceID]);

	tangent = normalize(tangent);
	binormal = normalize(binormal);

	output.tangent = float4(tangent,0);
	output.binormal = float4(binormal,0);*/
#endif

	return output;
}
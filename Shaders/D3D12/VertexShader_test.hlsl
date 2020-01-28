struct VSIn
{
    float3 pos : POSITION;
float3 color	: COLOR;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

cbuffer CB : register(b0)
{
	float4 diffuse_perMaterial;
	float4 translate_perObject[64];
}

VSOut main(VSIn input, uint index : SV_VertexID, uint instanceID : SV_InstanceID)
{
	VSOut output	= (VSOut)0;
 
output.pos = float4(input.pos, 1.0f);
output.pos = output.pos + translate_perObject[instanceID];
    

output.color = diffuse_perMaterial;

	return output;
}
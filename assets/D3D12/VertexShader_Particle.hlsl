RWBuffer<float3> particles : register(u0);

struct VSOut
{
	float4 pos : WPOS;
	float4 color : COLOR;
};

VSOut main(uint index : SV_VertexID)
{
	VSOut output = (VSOut)0;
 
	uint arr_id = index * 3;
	
	float x = particles[arr_id];
	float y = particles[arr_id+1];
	float z = particles[arr_id+2];
	
	output.pos = float4(x, y, z, 1.0f);
	output.color = float4((index % 1000) / 501, 0.0f , 0.0f, 1.0f);
	//output.pos = mul(viewPerspective, mul(sr[matrixIndex + instanceID], float4(input.pos, 1.0f)));

	return output;
}
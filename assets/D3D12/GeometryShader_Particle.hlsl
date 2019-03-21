struct VSOut
{
	float4 pos : WPOS;
	float4 color : COLOR;
};

struct GSOut
{
    float4 pos : SV_POSITION;
	float4 color : COLOR;
};

cbuffer CB : register(b0)
{
	float4x4 viewPerspective;
}

[maxvertexcount(3)]
void main(point VSOut input[1], inout TriangleStream<GSOut> OutputStream)
{
	GSOut output[3];
	float4 p = input[0].pos;

	float3 center = float3(0, 0, 0);
	float3 up = float3(0, 1, 0) * 0.1;

	float3 dir = p.xyz - center;
	dir = normalize(dir);
	float3 left = cross(float3(0, 1, 0), dir.xyz)*0.1;
	//float3 left = float3(0.01, 0, 0);

	output[0].pos = float4(p.xyz + left + up,1.0f);
	output[0].pos = mul(viewPerspective, output[0].pos);
	output[0].color = input[0].color;

	output[1].pos = float4(p.xyz - left + up, 1.0f);
	output[1].pos = mul(viewPerspective, output[1].pos);
	output[1].color = input[0].color;

	output[2].pos = float4(p.xyz + left - up, 1.0f);
	output[2].pos = mul(viewPerspective, output[2].pos);
	output[2].color = input[0].color;

	/*
	output[0].pos = float4(p.x - 0.01f, p.y + 0.01f, p.z, 1.0f);
	output[0].pos = mul(viewPerspective, output[0].pos);

	output[1].pos = float4(p.x - 0.01f, p.y - 0.01f, p.z, 1.0f);
	output[1].pos = mul(viewPerspective, output[1].pos);

	output[2].pos = float4(p.x + 0.01f, p.y + 0.01f, p.z, 1.0f);
	output[2].pos = mul(viewPerspective, output[2].pos);
	*/

	OutputStream.Append(output[0]);
	OutputStream.Append(output[1]);
	OutputStream.Append(output[2]);
	
}
struct VSOut
{
	float4 pos : WPOS;
};

struct GSOut
{
    float4 pos : SV_POSITION;
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

	output[0].pos = float4(p.x - 0.01f, p.y + 0.01f, p.z, 1.0f);
	output[0].pos = mul(viewPerspective, output[0].pos);

	output[1].pos = float4(p.x - 0.01f, p.y - 0.01f, p.z, 1.0f);
	output[1].pos = mul(viewPerspective, output[1].pos);

	output[2].pos = float4(p.x + 0.01f, p.y + 0.01f, p.z, 1.0f);
	output[2].pos = mul(viewPerspective, output[2].pos);


	OutputStream.Append(output[0]);
	OutputStream.Append(output[1]);
	OutputStream.Append(output[2]);
	
}
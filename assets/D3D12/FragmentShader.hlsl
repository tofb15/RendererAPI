Texture2D g_texture[] : register(t0);

sampler samp : register(s0);

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

//cbuffer CB : register(b0)
//{
//	int textureStart;
//}

float timeWaster(float input)
{
	float output = input;
	output += sin(input / sqrt(17));
	output *= cos(output / sqrt(17)) / 13;
	return output;
}


float4 main(VSOut input) : SV_TARGET0
{
	float4 textureColor;
	float3 lightDir = float3(-1.0f, 1.0f, -0.5f);

#ifdef TEXTCOORD

#ifdef NMAP
 
	float4 bumpMapColor;
	float3 bumpNormal;

	textureColor = g_texture[0 + input.instanceID * 2].Sample(samp, input.uv);
	//textureColor = float4(1,1,1,1);
	bumpMapColor = g_texture[1 + input.instanceID * 2].Sample(samp, input.uv);
	// Change interval from (0, 1) to (-1, 1)
	bumpMapColor = bumpMapColor * 2.0f - 1.0f;

	// Add up vectors and their influence
	bumpNormal = bumpMapColor.x * input.tangent + bumpMapColor.y * input.binormal + bumpMapColor.z * input.normal.xyz;
	bumpNormal = normalize(bumpNormal);

	textureColor = saturate(textureColor * saturate(dot(lightDir, bumpNormal))) + textureColor * 0.01;
	//textureColor = float4(bumpNormal.x,0, bumpNormal.z,0);// saturate(textureColor * dot(lightDir, bumpNormal)) + textureColor * 0.1;
	//textureColor.r = 1.0f;
	//textureColor.w = 0.7f;

#else
	textureColor = g_texture[input.instanceID].Sample(samp, input.uv);

	#ifdef NORMAL

	textureColor = saturate(textureColor * dot(lightDir, input.normal.xyz)) + textureColor * 0.01;
	//textureColor.w = 0.6f;


	/*for (int i = 0; i < 100000; i++)
	{
		finalColor.w += timeWaster(finalColor.w);
	}
	saturate(finalColor);*/

	#endif
#endif
		
#elif defined(NORMAL)
    textureColor = -1*float4(input.normal.xyz, 1.0f);
#else
    textureColor = input.color;//float4(1, 0, 1, 1);
#endif
	//textureColor = float4(input.normal.xyz, 1.0f);

	return textureColor;
}
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
cbuffer CBBig : register(b1)
{
	float4x4 worlds1[128];
	float4x4 worlds2[128];
	float4x4 worlds3[128];
	float4x4 worlds4[128];
	float4x4 worlds5[128];
	float4x4 worlds6[128];
	float4x4 worlds7[128];
	float4x4 worlds8[128];
	float4x4 worlds9[128];
	float4x4 worlds10[128];

	float4x4 worlds11[128];
	float4x4 worlds12[128];
	float4x4 worlds13[128];
	float4x4 worlds14[128];
	float4x4 worlds15[128];
	float4x4 worlds16[128];
	float4x4 worlds17[128];
	float4x4 worlds18[128];
	float4x4 worlds19[128];
	float4x4 worlds20[128];

	float4x4 worlds21[128];
	float4x4 worlds22[128];
	float4x4 worlds23[128];
	float4x4 worlds24[128];
	float4x4 worlds25[128];
	float4x4 worlds26[128];
	float4x4 worlds27[128];
	float4x4 worlds28[128];
	float4x4 worlds29[128];
	float4x4 worlds30[128];

	float4x4 worlds41[128];
	float4x4 worlds42[128];
	float4x4 worlds43[128];
	float4x4 worlds44[128];
	float4x4 worlds45[128];
	float4x4 worlds46[128];
	float4x4 worlds47[128];
	float4x4 worlds48[128];
	float4x4 worlds49[128];
	float4x4 worlds50[128];

	float4x4 worlds51[128];
	float4x4 worlds52[128];
	float4x4 worlds53[128];
	float4x4 worlds54[128];
	float4x4 worlds55[128];
	float4x4 worlds56[128];
	float4x4 worlds57[128];
	float4x4 worlds58[128];
	float4x4 worlds59[128];
	float4x4 worlds60[128];

	float4x4 worlds61[128];
	float4x4 worlds62[128];
	float4x4 worlds63[128];
	float4x4 worlds64[128];
	float4x4 worlds65[128];
	float4x4 worlds66[128];
	float4x4 worlds67[128];
	float4x4 worlds68[128];
	float4x4 worlds69[128];
	float4x4 worlds70[128];

	float4x4 worlds31[128];
	float4x4 worlds32[128];
	float4x4 worlds33[128];
	float4x4 worlds34[128];
	float4x4 worlds35[128];
	float4x4 worlds36[128];
	float4x4 worlds37[128];
	float4x4 worlds38[128];
	float4x4 worlds39[128];
	float4x4 worlds40[128];

	float4x4 worlds71[128];
	float4x4 worlds72[128];
	float4x4 worlds73[128];
	float4x4 worlds74[128];
	float4x4 worlds75[128];
	float4x4 worlds76[128];
	float4x4 worlds77[128];
	float4x4 worlds78[128];
	float4x4 worlds79[128];
	float4x4 worlds80[128];

	//float4x4 worlds81[128];
	//float4x4 worlds82[128];
	//float4x4 worlds83[128];
	//float4x4 worlds84[128];
	//float4x4 worlds85[128];
	//float4x4 worlds86[128];
	//float4x4 worlds87[128];
	//float4x4 worlds88[128];
	//float4x4 worlds89[128];
	//float4x4 worlds90[128];

	//float4x4 worlds91[128];
	//float4x4 worlds92[128];
	//float4x4 worlds93[128];
	//float4x4 worlds94[128];
	//float4x4 worlds95[128];
	//float4x4 worlds96[128];
	//float4x4 worlds97[128];
	//float4x4 worlds98[128];
	//float4x4 worlds99[128];
	//float4x4 worlds100[128];

}


VSOut main(VSIn input, uint index : SV_VertexID, uint instanceID : SV_InstanceID)
{
	VSOut output = (VSOut)0;
   // output.pos = mul(float4(input.pos, 1.0f), viewPerspective);
	output.pos = mul(mul(float4(input.pos, 1.0f), worlds1[matrixIndex]), viewPerspective);

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
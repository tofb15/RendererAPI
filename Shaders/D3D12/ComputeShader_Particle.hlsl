RWBuffer<float3> particles : register(u0);

cbuffer CB : register(b0)
{
	float time;
}

[numthreads(1000, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{

uint arrayPos = (DTid.x * 3);

//particles[arrayPos] = sin(time + DTid.x) * 0.5;
//particles[arrayPos + 1] = cos(DTid.x);

particles[arrayPos] =     sin(DTid.x * 0.00001 + time * 0.5) * 100;
particles[arrayPos + 1] = abs(sin(DTid.x) * cos(time) * 200);
particles[arrayPos + 2] = cos(DTid.x * 0.00001 + time * 0.5) * 100;

}
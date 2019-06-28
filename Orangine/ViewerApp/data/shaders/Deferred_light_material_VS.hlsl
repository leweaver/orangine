//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float3 vPosition    : POSITION;
};

struct VS_OUTPUT
{
	float2 vTexCoord0   : TEXCOORD0;
	float4 vPosition    : SV_POSITION;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;
	Output.vPosition  = float4(Input.vPosition, 1);
	Output.vTexCoord0 = float2(Input.vPosition.xy * .5 + .5) * float2(1, -1) + float2(0, 1);
	return Output;
}

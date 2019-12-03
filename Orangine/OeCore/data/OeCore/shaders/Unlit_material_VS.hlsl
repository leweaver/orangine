//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer cbPerObject : register(b0)
{
	matrix        g_mWorldViewProjection  : packoffset(c0);
	float4        g_baseColor             : packoffset(c4);
};

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 vPosition    : POSITION;
};

struct VS_OUTPUT
{
	float4 vColor       : COLOR0;
	float4 vPosition    : SV_POSITION;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;
	
	Output.vPosition = mul(g_mWorldViewProjection, Input.vPosition);
	Output.vPosition = float4(Output.vPosition.xyz / Output.vPosition.w, 1);

	Output.vColor = g_baseColor;

	return Output;
}

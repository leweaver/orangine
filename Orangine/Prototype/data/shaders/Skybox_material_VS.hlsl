//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer cbPerObject : register(b0)
{
	matrix        g_mWorldViewProjection  : packoffset(c0);

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
	float4 vPosition    : SV_POSITION;
	float3 vPositionL   : POSITION;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.vPositionL = Input.vPosition.xyz;
	Output.vPosition = mul(float4(Input.vPosition.xyz, 1), g_mWorldViewProjection);

	return Output;
}

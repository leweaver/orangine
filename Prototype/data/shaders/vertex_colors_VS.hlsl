
//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer cbPerObject : register(b0)
{
	matrix        g_mViewProjection       : packoffset(c0);
	matrix        g_mWorld                : packoffset(c4);

};

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 vPosition    : POSITION;
	float4 vColor       : COLOR0;

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
	matrix mWorldViewProjection = mul(g_mWorld, g_mViewProjection);
	Output.vPosition = mul(Input.vPosition, mWorldViewProjection);
	Output.vColor = Input.vColor;

	return Output;
}

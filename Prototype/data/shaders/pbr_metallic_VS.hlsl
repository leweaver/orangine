
//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer cbPerObject : register(b0)
{
	matrix        g_mViewProjection       : packoffset(c0);
	matrix        g_mWorld                : packoffset(c4);
	float4        g_baseColor             : packoffset(c8);
};

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 vPosition    : POSITION;
	float3 vNormal      : NORMAL;
	float4 vTangent     : TANGENT;
	float2 vTexCoord0   : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 vColor       : COLOR0;
	float3 vNormal      : NORMAL0;
	float3 vWorldNormal : NORMAL1;
	float4 vTangent     : TANGENT;
	float2 vTexCoord0   : TEXCOORD0;
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
	Output.vNormal = Input.vNormal;
	Output.vWorldNormal = mul(Input.vNormal, g_mWorld);
	Output.vTangent = Input.vTangent;
	Output.vTexCoord0 = Input.vTexCoord0;
	Output.vColor = g_baseColor;


	return Output;
}


//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer constants : register(b0)
{
	matrix        g_mWorldViewProjection  : packoffset(c0);
	matrix        g_mWorldView            : packoffset(c4);
	matrix        g_mWorld                : packoffset(c8);
	matrix        g_mWorldInvTranspose    : packoffset(c12);
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
	float3 vNormal      : NORMAL0;
	float3 vWorldNormal : NORMAL1;
	float4 vTangent     : TANGENT0;
	float3 vWorldTangent: TANGENT1;
	float2 vTexCoord0   : TEXCOORD0;
	float4 vClipPosition: TEXCOORD1;
	float4 vPosition    : SV_POSITION;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;
	Output.vPosition = mul(float4(Input.vPosition.xyz, 1), g_mWorldViewProjection);
	Output.vClipPosition = mul(Input.vPosition, g_mWorldView);

	/*
	// Remove any scaling from the world matrix by normalizing each column
	float3 A = normalize(float3(g_mWorld._11, g_mWorld._12, g_mWorld._13));
	float3 B = normalize(float3(g_mWorld._21, g_mWorld._22, g_mWorld._23));
	float3 C = normalize(float3(g_mWorld._31, g_mWorld._32, g_mWorld._33));
	float3x3 worldRot = {
		A[0], B[0], C[0],
		A[1], B[1], C[1],
		A[2], B[2], C[2],
	};
	*/

	Output.vNormal = Input.vNormal;
	Output.vTangent = Input.vTangent;

	// Normalize as the world matrix may have scaling applied.
	// TODO: does this work with non uniform scaling, or do we need to remove scaling from the matrix?
	Output.vWorldNormal = normalize(mul(Input.vNormal, g_mWorldInvTranspose));
	Output.vWorldTangent = normalize(mul(Input.vTangent.xyz, g_mWorld));

	Output.vTexCoord0 = Input.vTexCoord0;

	return Output;
}

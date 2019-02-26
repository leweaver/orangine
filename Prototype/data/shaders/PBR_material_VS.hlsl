
//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer constants : register(b0)
{
	matrix        g_mWorldViewProjection  : packoffset(c0);
	matrix        g_mWorldView            : packoffset(c4);
	matrix        g_mWorld                : packoffset(c8);
	matrix        g_mWorldInvTranspose    : packoffset(c12);
    float4        g_morphWeights0         : packoffset(c16);
    float4        g_morphWeights1         : packoffset(c17);
};

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 vPosition    : POSITION;
#if VB_NORMAL
	float3 vNormal      : NORMAL;
#endif
#if VB_TANGENT
	float4 vTangent     : TANGENT;
#endif
#if VB_TEXCOORD0
	float2 vTexCoord0   : TEXCOORD0;
#endif
#if VB_MORPH
    VB_MORPH_INPUTS
#endif
};

struct VS_OUTPUT
{
#if VB_NORMAL
	float3 vNormal      : NORMAL0;
	float3 vWorldNormal : NORMAL1;
#endif
#if VB_TANGENT
	float4 vTangent     : TANGENT0;
	float3 vWorldTangent: TANGENT1;
#endif
    float4 vClipPosition: TEXCOORD0;
#if VB_TEXCOORD0
    float2 vTexCoord0   : TEXCOORD1;
#endif
	float4 vPosition    : SV_POSITION;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;

#if VB_MORPH
    VB_MORPH_WEIGHTS_CALC
#else
    float3 morphedPosition = Input.vPosition.xyz;
#if VB_NORMAL
    float3 morphedNormal = Input.vNormal;
#endif
#if VB_TANGENT
    float4 morphedTangent = Input.vTangent;
#endif
#endif

	Output.vPosition = mul(float4(morphedPosition.xyz, 1), g_mWorldViewProjection);
	Output.vClipPosition = mul(float4(morphedPosition.xyz, 1), g_mWorldView);

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

#if VB_NORMAL
	Output.vNormal = morphedNormal;
    // Normalize as the world matrix may have scaling applied.
    Output.vWorldNormal = normalize(mul(morphedNormal, g_mWorldInvTranspose));
#endif

#if VB_TANGENT
	Output.vTangent = morphedTangent;
    // Normalize as the world matrix may have scaling applied.
    Output.vWorldTangent = normalize(mul(morphedTangent.xyz, g_mWorld));
#endif

#if VB_TEXCOORD0
	Output.vTexCoord0 = Input.vTexCoord0;
#endif

	return Output;
}

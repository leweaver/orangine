
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

cbuffer skinnedConstants : register(b1)
{
    matrix        g_boneTransforms[96];
}

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
#if VB_SKINNED
    uint4 vJoints       : BLENDINDICES;
    float4 vWeights     : BLENDWEIGHTS;
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

#if VB_SKINNED

    float3 positionL = float3(0, 0, 0);
#if VB_NORMAL
    float3 normalL = float3(0, 0, 0);
#endif
#if VB_TANGENT
    float3 tangentL = float3(0, 0, 0);
#endif

    for (int i = 0; i < 4; ++i) {
        positionL += Input.vWeights[i] *
            mul(float4(Input.vPosition.xyz, 1.0f),
                g_boneTransforms[Input.vJoints[i]]).xyz;

#if VB_NORMAL
        normalL += Input.vWeights[i] *
            mul(Input.vNormal.xyz,
                (float3x3)g_boneTransforms[Input.vJoints[i]]);
#endif
#if VB_TANGENT
        tangentL += Input.vWeights[i] *
            mul(Input.vTangent.xyz,
                (float3x3)g_boneTransforms[Input.vJoints[i]]);
#endif

    }
#else
    float3 positionL = Input.vPosition.xyz;
#if VB_NORMAL
    float3 normalL = Input.vNormal;
#endif
#if VB_TANGENT
    float3 tangentL = Input.vTangent.xyz;
#endif
#endif

#if VB_MORPH
    VB_MORPH_WEIGHTS_CALC
#endif

	Output.vPosition = mul(float4(positionL, 1.0f), g_mWorldViewProjection);
	Output.vClipPosition = mul(float4(positionL, 1.0f), g_mWorldView);

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
	Output.vNormal = normalL;
    // Normalize as the world matrix may have scaling applied.
    Output.vWorldNormal = normalize(mul(normalL.xyz, (float3x3)g_mWorldInvTranspose));
#endif

#if VB_TANGENT
	Output.vTangent = float4(tangentL, Input.vTangent.w);
    // Normalize as the world matrix may have scaling applied.
    Output.vWorldTangent = normalize(mul(tangentL.xyz, (float3x3)g_mWorld));
#endif

#if VB_TEXCOORD0
	Output.vTexCoord0 = Input.vTexCoord0;
#endif

	return Output;
}

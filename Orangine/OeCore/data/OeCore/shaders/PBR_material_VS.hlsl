
//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer constants : register(b0)
{
    matrix        g_mWorldViewProjection;
    matrix        g_mWorld;
    matrix        g_mViewProjection;
    matrix        g_mWorldInvTranspose;
    float4        g_morphWeights0;
    float4        g_morphWeights1;
};

#if VB_SKINNED
cbuffer skinnedConstants : register(b2)
{
    matrix        g_boneTransforms[96];
}
#endif

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
    uint4 vJoints       : BONEINDICES;
    float4 vWeights     : WEIGHTS;
#endif
#if VB_MORPH
    VB_MORPH_INPUTS
#endif
};

struct VS_OUTPUT
{
#if VB_NORMAL
	float3 vWorldNormal : NORMAL0;
#endif
#if VB_TANGENT
	float4 vTangent     : TANGENT0;
	float3 vWorldTangent: TANGENT1;
#endif
#if VB_TEXCOORD0
    float2 vTexCoord0   : TEXCOORD0;
#endif
	float4 vPosition    : SV_POSITION;
};

// https://github.com/glslify/glsl-inverse/blob/master/index.glsl
float3x3 inverseMat3(float3x3 inMatrix) {
	// For skinned meshes, need to calculate inverse world in the vertex shader. Oof! 
    float a00 = inMatrix[0][0], a01 = inMatrix[0][1], a02 = inMatrix[0][2];
    float a10 = inMatrix[1][0], a11 = inMatrix[1][1], a12 = inMatrix[1][2];
    float a20 = inMatrix[2][0], a21 = inMatrix[2][1], a22 = inMatrix[2][2];

    float b01 = a22 * a11 - a12 * a21;
    float b11 = -a22 * a10 + a12 * a20;
    float b21 = a21 * a10 - a11 * a20;

    float det = a00 * b01 + a01 * b11 + a02 * b21;

    return float3x3(b01, (-a22 * a01 + a02 * a21), (a12 * a01 - a02 * a11),
        b11, (a22 * a00 - a02 * a20), (-a12 * a00 + a02 * a10),
        b21, (-a21 * a00 + a01 * a20), (a11 * a00 - a01 * a10)) / det;
}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;

    float3 positionL = Input.vPosition.xyz;
#if VB_NORMAL
    float3 normalL = Input.vNormal;
#endif
#if VB_TANGENT
    float3 tangentL = Input.vTangent.xyz;
#endif

#if VB_MORPH
    VB_MORPH_WEIGHTS_CALC
#endif

#if VB_SKINNED
    float4x4 skinInfluence =
        g_boneTransforms[Input.vJoints[0]] * Input.vWeights[0] +
        g_boneTransforms[Input.vJoints[1]] * Input.vWeights[1] +
        g_boneTransforms[Input.vJoints[2]] * Input.vWeights[2] +
        g_boneTransforms[Input.vJoints[3]] * Input.vWeights[3];

	float4x4 world = mul(g_mWorld, skinInfluence);
	float4x4 worldViewProjection = mul(g_mViewProjection, world);
	float3x3 worldInvTranspose = transpose(inverseMat3((float3x3)world));
#else
	float4x4 world = g_mWorld;
	float4x4 worldViewProjection = g_mWorldViewProjection;
	float3x3 worldInvTranspose = float3x3(
        g_mWorldInvTranspose[0].xyz,
        g_mWorldInvTranspose[1].xyz,
        g_mWorldInvTranspose[2].xyz);
#endif
    
	Output.vPosition = mul(worldViewProjection, float4(positionL, 1.0f));

#if VB_NORMAL
    // Normalize as the world matrix may have scaling applied.
    Output.vWorldNormal = normalize(mul(worldInvTranspose, normalL.xyz));
#endif

#if VB_TANGENT
	Output.vTangent = float4(tangentL, Input.vTangent.w);
    // Normalize as the world matrix may have scaling applied.
    Output.vWorldTangent = normalize(mul((float3x3)world, tangentL.xyz));
#endif

#if VB_TEXCOORD0
	Output.vTexCoord0 = Input.vTexCoord0;
#endif

	return Output;
}

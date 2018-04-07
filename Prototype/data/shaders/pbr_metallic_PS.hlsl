#include "inc\utils.hlsl"

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer constants : register(b0)
{
	matrix        g_mWorld                : packoffset(c0);
	float4        g_baseColor             : packoffset(c4);
	float4        g_metallicRoughness     : packoffset(c5); // metallic, roughness
};

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
	float3 vNormal      : NORMAL0;
	float3 vWorldNormal : NORMAL1;
	float4 vTangent     : TANGENT0;
	float3 vWorldTangent: TANGENT1;
	float2 vTexCoord0   : TEXCOORD0;
};

struct PS_OUTPUT
{
	float4  Color  : SV_Target0; // xyz: Diffuse          w: metallic
	float4  Color1 : SV_Target1; // xyz: World normals    w: roughness
};


// Order of these samplers must match the enum order in PBRMaterial.h
#ifdef MAP_BASECOLOR
Texture2D baseColorTexture;
SamplerState baseColorSampler;
#endif

#ifdef MAP_METALLIC_ROUGHNESS
Texture2D metallicRoughnessTexture;
SamplerState metallicRoughnessSampler;
#endif

#ifdef MAP_NORMAL
Texture2D normalTexture;
SamplerState normalSampler;
#endif

//--------------------------------------------------------------------------------------
// Forward Declarations
//--------------------------------------------------------------------------------------
float3 NormalSampleToWorldNormal(float3 sampleT, float3 normalW, float3 tangentW, float tangentHandedness);

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
PS_OUTPUT PSMain(PS_INPUT input)
{
	PS_OUTPUT output;
		
#ifdef MAP_BASECOLOR
	float3 baseColor = SRGBtoLINEAR(baseColorTexture.Sample(baseColorSampler, input.vTexCoord0).xyz) * g_baseColor;
#else
	float3 baseColor = g_baseColor;
#endif

#ifdef MAP_METALLIC_ROUGHNESS
	float2 metallicRoughness = metallicRoughnessTexture.Sample(metallicRoughnessSampler, input.vTexCoord0).xy;
#else
	float2 metallicRoughness = g_metallicRoughness.xy;
#endif
	
#ifdef MAP_NORMAL
	float3 worldNormal = NormalSampleToWorldNormal(normalTexture.Sample(normalSampler, input.vTexCoord0).rgb, input.vWorldNormal, input.vWorldTangent, input.vTangent.w);
#else
	float3 worldNormal = input.vWorldNormal.xyz;
#endif

	output.Color = float4(baseColor, metallicRoughness.x);
	output.Color1 = float4(worldNormal * 0.5 + 0.5, metallicRoughness.y);

	return output;
}

float3 NormalSampleToWorldNormal(float3 sampleT, float3 normalW, float3 tangentW, float tangentHandedness) 
{
	float3 N = normalW;
	// After interpolation, N and T may not be orthonormal. This next line will remove any 'N'
	// component from the T vector.
	float3 T = normalize(tangentW - dot(tangentW, N) * N);
	float3 B = cross(N, T) * tangentHandedness;

	float3x3 TBN = float3x3(T, B, N);

	// Transform normal map sample from [0, -1] to [-1, 1] space
	sampleT = sampleT * 2.0 - 1.0;

	// Transform to world space
	return mul(sampleT, TBN);
}
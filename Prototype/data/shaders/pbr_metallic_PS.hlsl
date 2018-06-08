#include "inc\utils.hlsl"

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer cb_surface : register(b0)
{
	matrix        g_mWorld             : packoffset(c0);
	float4        g_baseColor          : packoffset(c4);
	float4        g_metallicRoughness  : packoffset(c5); // metallic, roughness
	float4        g_emissive           : packoffset(c6); // emissive color (RGB)
	float4        g_eyePosition        : packoffset(c7);
};

#if PS_PIPELINE_DEFERRED
#include "inc/pipeline_deferred.hlsl"
#elif PS_PIPELINE_STANDARD
#include "inc/pipeline_standard.hlsl"
#else
#error PS_PIPELINE_DEFERRED or PS_PIPELINE_STANDARD must be defined
#endif

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
	float3 vNormal        : NORMAL0;
	float3 vWorldNormal   : NORMAL1;
	float4 vTangent       : TANGENT0;
	float3 vWorldTangent  : TANGENT1;
	float2 vTexCoord0     : TEXCOORD0;
	float4 vWorldPosition : SV_POSITION;
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

#ifdef MAP_OCCLUSION
Texture2D occlusionTexture;
SamplerState occlusionSampler;
#endif

#ifdef MAP_EMISSIVE
Texture2D emissiveTexture;
SamplerState emissiveSampler;
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
	float4 baseColorSample = baseColorTexture.Sample(baseColorSampler, input.vTexCoord0);
	float4 baseColor = float4(SRGBtoLINEAR(baseColorSample.rgb) * g_baseColor.rgb, baseColorSample.a);
#else
	float4 baseColor = g_baseColor;
#endif

#ifdef ALPHA_MASK_VALUE
	if (baseColor.a < ALPHA_MASK_VALUE) {
		discard;
	}
	else {
		baseColor.a = 1.0f;
	}
#endif

#ifdef MAP_METALLIC_ROUGHNESS
	float4 metallicRoughnessSample = metallicRoughnessTexture.Sample(metallicRoughnessSampler, input.vTexCoord0);
	float2 metallicRoughness = float2(metallicRoughnessSample.b, metallicRoughnessSample.g) * g_metallicRoughness.xy;
#else
	float2 metallicRoughness = g_metallicRoughness.xy;
#endif
	
#ifdef MAP_NORMAL
	float3 worldNormal = NormalSampleToWorldNormal(normalTexture.Sample(normalSampler, input.vTexCoord0).rgb, input.vWorldNormal, input.vWorldTangent, input.vTangent.w);
#else
	float3 worldNormal = input.vWorldNormal.xyz;
#endif

#ifdef MAP_EMISSIVE
	float3 emissiveColor = emissiveTexture.Sample(emissiveSampler, input.vTexCoord0).rgb;
#else
	float3 emissiveColor = g_emissive.rgb;
#endif

#ifdef MAP_OCCLUSION
	float occlusionFactor = occlusionTexture.Sample(occlusionSampler, input.vTexCoord0).r;
#else
	float occlusionFactor = 1;
#endif

	return EncodeOutput(
		baseColor,
		metallicRoughness.x,
		worldNormal * 0.5 + 0.5,
		metallicRoughness.y,
		emissiveColor.rgb,
		occlusionFactor,
		input.vWorldPosition.xyz);

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
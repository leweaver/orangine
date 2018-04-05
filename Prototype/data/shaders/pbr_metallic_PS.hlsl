//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer constants : register(b0)
{
	matrix        g_mWorld                : packoffset(c0);
	float4        g_baseColor             : packoffset(c4);
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
	float4  Color  : SV_Target0; // xyz: Diffuse          w: Specular intensity
	float4  Color1 : SV_Target1; // xyz: World normals    w: Specular power
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

// Forward Declarations
float3 NormalSampleToWorldNormal(float3 sampleT, float3 normalW, float3 tangentW, float tangentHandedness);

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
PS_OUTPUT PSMain(PS_INPUT input)
{
	PS_OUTPUT output;
		
#ifdef MAP_BASECOLOR
	float3 baseColor = baseColorTexture.Sample(baseColorSampler, input.vTexCoord0).xyz;
#else
	float3 baseColor = float3(0, 0.5, 1.0);
#endif

#ifdef MAP_METALLIC_ROUGHNESS
	float3 metallicRoughness = metallicRoughnessTexture.Sample(metallicRoughnessSampler, input.vTexCoord0).xyz;
#endif
	
#ifdef MAP_NORMAL
	float3 worldNormal = NormalSampleToWorldNormal(normalTexture.Sample(normalSampler, input.vTexCoord0).rgb, input.vWorldNormal, input.vWorldTangent, input.vTangent.w);
#else
	float3 worldNormal = input.vWorldNormal.xyz;
#endif
	float specularIntensity = 1.0;
	float specularPower = 1.0 + (metallicRoughness.x * 0.0);

	output.Color = float4(baseColor, specularIntensity);
	output.Color1 = float4(worldNormal * 0.5 + 0.5, specularPower);

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
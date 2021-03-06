Texture2D g_envBrdfLUTexture;
Texture2D g_envDiffuseTexture;
TextureCube g_envSpecularTexture;
SamplerState g_envSampler;

Texture2DArray g_shadowMapDepthTexture;
Texture2DArray<uint2> g_shadowMapStencilTexture;
SamplerState g_shadowMapSampler;

#include "inc/lighting.hlsl"

cbuffer cb_lights : register(b1)
{
	Light g_lights[8];
	uint  g_lightCount;
};

struct PS_OUTPUT
{
	float4  Color  : SV_Target0; // rgb: Diffuse          a: alpha
};

PS_OUTPUT EncodeOutput(
	float4 baseColor,
	float metallic,
	float3 worldNormal,
	float roughness,
	float3 emissive,
	float occlusion,
	float3 worldPosition)
{
	PS_OUTPUT output;

	BRDFLightInputs brdf;
	brdf.metallic = metallic;
	brdf.roughness = roughness;
	brdf.occlusion = occlusion;
	brdf.worldPosition = worldPosition;
	brdf.worldNormal = worldNormal;
	brdf.baseColor = baseColor.rgb;

#ifdef DEBUG_LIGHTING_ONLY
	brdf.baseColor = float3(1, 1, 1);
#endif

	// Vector from surface point to camera
	brdf.eyeVectorW = normalize(g_eyePosition.xyz - brdf.worldPosition);

	LightingInputs li;
	li.NdotV = abs(dot(brdf.worldNormal, brdf.eyeVectorW)) + 0.001;

	// Iterate over the lights
	float3 finalColor = float3(0, 0, 0);
	for (uint lightIdx = 0; lightIdx < g_lightCount; ++lightIdx)
	{
		Light light = g_lights[lightIdx];

		brdf.lightType = light.type;
		brdf.lightIntensifiedColor = light.intensifiedColor.rgb;
		brdf.lightPosition = light.directionPosition;

		finalColor += BRDFLight(brdf, li);
	}
	
	// Emitted light
	finalColor += emissive;
	
	// Pre-multiply the alpha
	output.Color = float4(finalColor.rgb * baseColor.a, baseColor.a);

	return output;
}
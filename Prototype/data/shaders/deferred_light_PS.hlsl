#include "inc\lighting.hlsl"

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer main_cb : register(b0)
{
	matrix        g_viewMatrixInv;
	matrix        g_projMatrixInv;
	float4        g_eyePosition;
	bool          g_emittedEnabled;
};
cbuffer light_cb : register(b1)
{
	Light g_lights[8];
	uint  g_lightCount;
};

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------

struct PS_INPUT
{
	float2 vTexCoord0   : TEXCOORD0;
};

// rgb: Diffuse           
// a:   Specular intensity
Texture2D color0Texture : register(t0);
SamplerState color0Sampler : register(s0);

// rgb: World normals     
// a:   Specular power
Texture2D color1Texture : register(t1);
SamplerState color1Sampler : register(s1);

// rgb: Emissive color
// a:   Occlusion
Texture2D color2Texture : register(t2);
SamplerState color2Sampler : register(s2);

// r:   normalized depth    
// g:   Stencil
Texture2D depthTexture : register(t3);
SamplerState depthSampler : register(s3);

// Shadowmaps
Texture2D shadowMapTextures[8];
SamplerState shadowMapSamplers[8];

// Forward declarations
float3 worldPosFromDepth(float2 texCoord);
float3 PointLightVector(float3 lightPosition, float3 lightIntensifiedColor, float3 pixelPosition, float3 surfaceNormal_n);

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain(PS_INPUT input) : SV_TARGET
{
	// Inputs
	float4 color0 = color0Texture.Sample(color0Sampler, input.vTexCoord0);
	float4 color1 = color1Texture.Sample(color1Sampler, input.vTexCoord0);
	float4 color2 = color2Texture.Sample(color2Sampler, input.vTexCoord0);
	float  depth = depthTexture.Sample(depthSampler,   input.vTexCoord0).r;

	BRDFLightInputs brdf;
	brdf.emissiveColor = color2.rgb;
	brdf.metallic = color0.w;
	brdf.roughness = color1.w;
	brdf.occlusion = color2.a;
	brdf.worldPosition = worldPosFromDepth(input.vTexCoord0);
	brdf.worldNormal = color1.xyz * 2 - 1;
	brdf.baseColor = color0.rgb;

#ifdef DEBUG_LIGHTING_ONLY
	brdf.baseColor = float3(1, 1, 1);
#endif
		float3 worldPos = brdf.worldPosition;

	// Vector from surface point to camera
	brdf.eyeVectorW = normalize(g_eyePosition.xyz - brdf.worldPosition);

	// Iterate over the lights
	float3 finalColor = float3(0, 0, 0);
	for (uint lightIdx = 0; lightIdx < g_lightCount; ++lightIdx)
	{
		Light light = g_lights[lightIdx];

		brdf.lightType = light.type;
		brdf.lightIntensifiedColor = light.intensifiedColor.rgb;
		brdf.lightPosition = light.directionPosition;

		float3 lightColor = BRDFLight(brdf);

		if (light.shadowMapIndex != -1) {
			float4 shadowCoord = mul(float4(brdf.worldPosition, 1), light.shadowMapViewMatrix);
			shadowCoord = float4(shadowCoord.xyz / shadowCoord.w, 1.0);

			// Bring x,y from [-1, 1] to [0, 1]
			// TODO: Why do we need to y-flip here?
			shadowCoord = shadowCoord * float4(0.5, -0.5, 1, 1) + 0.5;

			float shadowSample = shadowMapTextures[0].Sample(shadowMapSamplers[0], shadowCoord.rg).r;

			// TODO: shadowmap max depth from light param
			float shadowMaxDepth = 2.0f;
			float bias = 0.5f;

			shadowSample = shadowSample * shadowMaxDepth + bias;
			if (shadowCoord.z > shadowSample)
				lightColor = float3(0, 0, 0);

			if (brdf.baseColor.g > 0.1) {
				//finalColor = float3(shadowSample, 0, 0);
				//finalColor = float3(
				//	shadowCoord.x > 0 && shadowCoord.x < 1 ? shadowCoord.x * 0.5 + 0.5 : 0,
				//	shadowCoord.y > 0 && shadowCoord.y < 1 ? shadowCoord.y * 0.5 + 0.5 : 0,
				//	0);
				//finalColor = brdf.worldPosition;
			}
				
			//finalColor = float4(mul(float3(0.1f, 0.1f, 0.0), g_invWorldViewProj).xyz * 3, 0);
			//finalColor = g_invWorldViewProj._m00_m01_m02_m03;
		}

		finalColor += lightColor;
	}

	if (g_emittedEnabled)
	{
		brdf.lightType = LIGHT_TYPE_EMITTED;
		finalColor += BRDFLight(brdf);
	}

#ifdef DEBUG_NO_LIGHTING
	return float4(color0.rgb, 1);
#elif DEBUG_DISPLAY_METALLIC_ROUGHNESS
	return float4(metallic, roughness, 0, 1);
#elif DEBUG_DISPLAY_NORMALS
	return float4((color0.rgb * 0.001) + worldNormal * 0.5 + 0.5, 1);
#else
	return float4(finalColor, 1);
#endif
}

// https://stackoverflow.com/questions/32227283/getting-world-position-from-depth-buffer-value
float3 worldPosFromDepth(float2 texCoord)
{
	// Bring z into [-1, 1] space
	float depth = depthTexture.Sample(depthSampler, texCoord).r;

	// Get x/w and y/w from the viewport position
	float x = texCoord.x * 2 - 1;
	float y = (1 - texCoord.y) * 2 - 1;
	float z = depth;
	float4 clipSpacePosition = float4(x, y, z, 1.0f);
	float4 viewSpacePosition = mul(clipSpacePosition, g_projMatrixInv);

	// Perspective division (go from homogenious to 3d space)
	viewSpacePosition /= viewSpacePosition.w;

	// Convert from view to world space
	return mul(viewSpacePosition, g_viewMatrixInv).xyz;
}

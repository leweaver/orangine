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
Texture2DArray shadowMapDepthTexture : register(t4);
Texture2DArray<uint2> shadowMapStencilTexture : register(t5);
SamplerState shadowMapSampler : register(s4);

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
	brdf.metallic = color0.w;
	brdf.roughness = color1.w;
	brdf.occlusion = color2.a;
	brdf.worldPosition = worldPosFromDepth(input.vTexCoord0);
	brdf.worldNormal = color1.xyz * 2 - 1;
	brdf.baseColor = color0.rgb;
	float3 emissive = color2.rgb;

#ifdef DEBUG_LIGHTING_ONLY
	brdf.baseColor = float3(1, 1, 1);
#endif

	// Vector from surface point to camera
	brdf.eyeVectorW = normalize(g_eyePosition.xyz - brdf.worldPosition);

	float3 finalColor = float3(0, 0, 0);
	ShadowSampleInputs ssi;
	ssi.worldPosition = brdf.worldPosition;

	// Iterate over the lights
	for (uint lightIdx = 0; lightIdx < g_lightCount; ++lightIdx)
	{
		Light light = g_lights[lightIdx];

		brdf.lightType = light.type;
		brdf.lightIntensifiedColor = light.intensifiedColor.rgb;
		brdf.lightPosition = light.directionPosition;
		
		float3 lightColor = BRDFLight(brdf);
		if (light.shadowMapIndex != -1) {
			ssi.lightColor = lightColor;
			ssi.shadowMapDepthTexture = shadowMapDepthTexture;
			ssi.shadowMapStencilTexture = shadowMapStencilTexture;
			ssi.shadowMapSampler = shadowMapSampler;
			ssi.shadowMapArrayIndex = light.shadowMapIndex;
			ssi.shadowMapViewMatrix = light.shadowMapViewMatrix;
			ssi.shadowMapDepth = light.shadowMapDepth;
			ssi.shadowMapBias = light.shadowMapBias;
			finalColor += Shadow(ssi);
		}
		else {
			finalColor += lightColor;
		}
	}

	// TODO: turn into an ifdef?
	if (g_emittedEnabled)
	{
		finalColor += emissive;
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

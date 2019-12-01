// rgb: Diffuse           
// a:   Specular intensity
Texture2D color0Texture;
SamplerState color0Sampler;

// rgb: World normals     
// a:   Specular power
Texture2D color1Texture;
SamplerState color1Sampler;

// rgb: Emissive color
// a:   Occlusion
Texture2D color2Texture;
SamplerState color2Sampler;

// r:   normalized depth
Texture2D depthTexture;
SamplerState depthSampler;

#ifdef MAP_IBL
Texture2D g_envBrdfLUTexture;
TextureCube g_envDiffuseTexture;
TextureCube g_envSpecularTexture;
SamplerState g_envSampler;
#endif

#ifdef MAP_SHADOWMAP_ARRAY
// Shadowmaps
// r: depth
Texture2DArray g_shadowMapDepthTexture;
// r: unused
// g: stencil
Texture2DArray<uint2> g_shadowMapStencilTexture;
SamplerState g_shadowMapSampler;
#endif

#include "inc\utils.hlsl"
#include "inc\lighting.hlsl"

#ifdef MAP_IBL
#include "inc\irradiance.hlsl"
#endif

#ifdef MAP_SHADOWMAP_ARRAY
#include "inc\shadow.hlsl"
#endif

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
    // Transform normal map sample from [0, -1] to [-1, 1] space
	brdf.worldNormal = color1.xyz * 2 - 1;
    // TODO: In theory, the normal coming in here should be normalized. But, for some reason, it isn't.
    brdf.worldNormal = normalize(brdf.worldNormal);
	brdf.baseColor = color0.rgb;
	float3 emissive = color2.rgb;

	float3 f0 = float3(0.04, 0.04, 0.04);
	float3 diffuseColor = brdf.baseColor.rgb * (float3(1.0, 1.0, 1.0) - f0);
	diffuseColor *= 1.0 - brdf.metallic;
	float3 specularColor = lerp(f0, brdf.baseColor.rgb, brdf.metallic);

#ifdef DEBUG_LIGHTING_ONLY
	brdf.baseColor = float3(1, 1, 1);
#endif

	// Vector from surface point to camera
	brdf.eyeVectorW = normalize(g_eyePosition.xyz - brdf.worldPosition);

	float3 finalColor = float3(0, 0, 0);
#ifdef MAP_SHADOWMAP_ARRAY
	ShadowSampleInputs ssi;
	ssi.worldPosition = brdf.worldPosition;
#endif

	LightingInputs li;
	li.NdotV = abs(dot(brdf.worldNormal, brdf.eyeVectorW)) + 0.001;
    
	// Iterate over the lights
	for (uint lightIdx = 0; lightIdx < g_lightCount; ++lightIdx)
    //uint lightIdx = 0;
	{
		Light light = g_lights[lightIdx];

		brdf.lightType = light.type;
		brdf.lightIntensifiedColor = light.intensifiedColor.rgb;
		brdf.lightPosition = light.directionPosition;
		
		float3 lightColor = BRDFLight(brdf, li);
#ifdef MAP_SHADOWMAP_ARRAY
		if (light.shadowMapIndex != -1) {
			ssi.lightColor = lightColor;
			ssi.shadowMapArrayIndex = light.shadowMapIndex;
			ssi.shadowMapViewMatrix = light.shadowMapViewMatrix;
			ssi.shadowMapBias = light.shadowMapBias;
			ssi.shadowMapDimension = light.shadowMapDimension;
			finalColor += Shadow(ssi);
		}
		else {
			finalColor += lightColor;
		}
#else
		finalColor += lightColor;
#endif
	}


#if DEBUG_DISPLAY_LIGHTING_ONLY
	float3 lightingColor = finalColor;
#endif

	// TODO: turn into an ifdef?
	if (g_emittedEnabled)
	{
		finalColor += emissive;
	}

    // Environment map
#ifdef MAP_IBL
    BRDFEnvInputs brdfEnvInputs;
    brdfEnvInputs.surfaceNormal = brdf.worldNormal;
    brdfEnvInputs.reflection = -normalize(reflect(brdf.eyeVectorW, brdf.worldNormal));
    brdfEnvInputs.roughness = brdf.roughness;
    brdfEnvInputs.diffuseColor = diffuseColor;
    brdfEnvInputs.specularColor = specularColor;
    brdfEnvInputs.scaleIBLAmbient = float2(1.0, 1.0);

    float3 envContribution = BRDFEnv(brdfEnvInputs, li);
    finalColor += envContribution;
#endif

#ifdef DEBUG_NO_LIGHTING
	return float4(color0.rgb, 1);
#elif DEBUG_DISPLAY_METALLIC_ROUGHNESS
	return float4(metallic, roughness, 0, 1);
#elif DEBUG_DISPLAY_NORMALS
	return float4((finalColor.rgb * 0.001) + brdf.worldNormal * 0.5 + 0.5, 1);
#elif DEBUG_DISPLAY_WORLD_POSITION
	return float4((finalColor.rgb * 0.001) + brdf.worldPosition, 1);
#elif DEBUG_DISPLAY_LIGHTING_ONLY
	return float4((finalColor.rgb * 0.001) + lightingColor, 1);
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
	float4 viewSpacePosition = mul(g_projMatrixInv, clipSpacePosition);

	// Perspective division (go from homogenious to 3d space)
	viewSpacePosition /= viewSpacePosition.w;

	// Convert from view to world space
	return mul(g_viewMatrixInv, viewSpacePosition).xyz;
}

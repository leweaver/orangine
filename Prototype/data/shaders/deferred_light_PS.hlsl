#include "inc\lighting.hlsl"

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer constants : register(b0)
{
	matrix        g_mMatInvProjection;
	// --
	float3        g_lightPosition;
	int			  g_lightType;
	// --
	float4        g_eyePosition;
	// --
	float3        g_lightIntensifiedColor;
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

// r:   normalized depth    
// g:   Stencil
Texture2D depthTexture : register(t2);
SamplerState depthSampler : register(s2);

// Forward declarations
float3 VSPositionFromDepth(float2 vTexCoord);
float3 PointLightVector(float3 lightPosition, float3 lightIntensifiedColor, float3 pixelPosition, float3 surfaceNormal_n);

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain(PS_INPUT input) : SV_TARGET
{
	// Inputs
	float4 color0 = color0Texture.Sample(color0Sampler, input.vTexCoord0);
	float4 color1 = color1Texture.Sample(color1Sampler, input.vTexCoord0);
	float  depth  = depthTexture.Sample(depthSampler,   input.vTexCoord0).r;

	// Decoded inputs
	float3 vsPosition = VSPositionFromDepth(input.vTexCoord0);
	float3 worldNormal = color1.xyz * 2 - 1;
	float metallic = color0.w;
	float roughness = color1.w;

#ifdef DEBUG_LIGHTING_ONLY
	float3 baseColor = float3(1, 1, 1);
#else
	float3 baseColor = color0.rgb;
#endif

	// Lighting Parameters
	float3 eyeVectorW = normalize(vsPosition - g_eyePosition.xyz);
	float3 lightVector;
	float3 lightIntensifiedColor = g_lightIntensifiedColor.rgb;
	if (g_lightType == 1)
	{
		float3 posDifference = vsPosition - g_lightPosition.xyz;
		lightIntensifiedColor = lightIntensifiedColor / length(posDifference);
		lightVector = normalize(posDifference);
	}
	else
	{
		lightVector = g_lightPosition.xyz;
	}

	// TODO: I guess we should apply simlpy lighting to the base color first? 
	//baseColor *= max(0, lightIntensifiedColor * -dot(lightVector, worldNormal) / 3.14159);

	// Final lighting calculations
	float3 finalColor = lightIntensifiedColor * BRDF(metallic, roughness, baseColor,
		eyeVectorW, lightVector, worldNormal);


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

float3 VSPositionFromDepth(float2 vTexCoord)
{
	// Get the depth value for this pixel
	float z = depthTexture.Sample(depthSampler, vTexCoord).r;

	// Get x/w and y/w from the viewport position
	float x = vTexCoord.x * 2 - 1;
	float y = (1 - vTexCoord.y) * 2 - 1;
	float4 vProjectedPos = float4(x, y, z, 1.0f);

	// Transform by the inverse projection matrix
	float4 vPositionVS = mul(vProjectedPos, g_mMatInvProjection);

	// Divide by w to get the view-space position
	return vPositionVS.xyz / vPositionVS.w;
}
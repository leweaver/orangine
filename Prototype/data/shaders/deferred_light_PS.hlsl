
//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer constants : register(b0)
{
	matrix        g_mMatInvProjection     : packoffset(c0);
	float3        g_lightDirection        : packoffset(c4);
	float3        g_lightIntensifiedColor : packoffset(c5);
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
float3 PointLight(float3 lightPosition, float3 lightColor, float lightIntensity, float3 pixelPosition, float3 surfaceNormal_n);
float3 DirLight(float3 lightDirection, float3 lightColor, float lightIntensity, float3 surfaceNormal_n);

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain(PS_INPUT input) : SV_TARGET
{
	float4 color0 = color0Texture.Sample(color0Sampler, input.vTexCoord0);
	float4 color1 = color1Texture.Sample(color1Sampler, input.vTexCoord0);
	float  depth  = depthTexture.Sample(depthSampler,   input.vTexCoord0).r;

	float3 vsPosition = VSPositionFromDepth(input.vTexCoord0);	
	float3 intensity = 0;
		
	intensity += DirLight(g_lightDirection.xyz, g_lightIntensifiedColor.rgb, 1, color1.xyz);

	return float4(color0.rgb * intensity, 1);
}

float3 PointLight(float3 lightPosition, float3 lightColor, float lightIntensity, float3 pixelPosition, float3 surfaceNormal_n)
{
	float3 posDifference = pixelPosition - lightPosition;
	lightIntensity = lightIntensity / length(posDifference);
	float3 lightDirection_n = normalize(posDifference);
	return max(0, lightIntensity * lightColor * -dot(lightDirection_n, surfaceNormal_n) / 3.14159);
}

float3 DirLight(float3 lightDirection, float3 lightColor, float lightIntensity, float3 surfaceNormal_n)
{
	return max(0, lightIntensity * lightColor * -dot(lightDirection, surfaceNormal_n) / 3.14159);
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
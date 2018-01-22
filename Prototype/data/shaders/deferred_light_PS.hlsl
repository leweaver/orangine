
//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

cbuffer constants : register(b0)
{
	matrix        g_mMatInvProjection     : packoffset(c0);
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
SamplerState color0Sampler : register(s1);

// rgb: World normals     
// a:   Specular power
Texture2D color1Texture : register(t1);
SamplerState color1Sampler : register(s1);

// r:   normalized depth    
// g:   Stencil
Texture2D depthTexture : register(t2);
SamplerState depthSampler : register(s2);

float3 VSPositionFromDepth(float2 vTexCoord);

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain(PS_INPUT input) : SV_TARGET
{
	float4 color0 = color0Texture.Sample(color0Sampler, input.vTexCoord0);
	float4 color1 = color1Texture.Sample(color1Sampler, input.vTexCoord0);
	float  depth  = depthTexture.Sample(depthSampler,   input.vTexCoord0).r;

	float3 vsPosition = VSPositionFromDepth(input.vTexCoord0);

	return float4(vsPosition, 1);
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
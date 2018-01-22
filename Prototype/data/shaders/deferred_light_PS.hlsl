//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
	float2 vTexCoord0   : TEXCOORD0;
};

// xyz: Diffuse          w: Specular intensity
Texture2D color0Texture : register(t0);
SamplerState color0Sampler : register(s1);

// xyz: World normals    w: Specular power
Texture2D color1Texture : register(t1);
SamplerState color1Sampler : register(s1);

// xyz: World normals    w: Specular power
Texture2D depthTexture : register(t2);
SamplerState depthSampler : register(s2);

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain(PS_INPUT input) : SV_TARGET
{
	float4 color0 = color0Texture.Sample(color0Sampler, input.vTexCoord0);
	float4 color1 = color1Texture.Sample(color1Sampler, input.vTexCoord0);
	float  depth  = depthTexture.Sample(depthSampler,   input.vTexCoord0).r;

	return float4(depth, 0, 0, 1);
}

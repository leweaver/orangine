//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
	float2 vTexCoord0   : TEXCOORD0;
};

// xyz: Diffuse          w: Specular intensity
Texture2D color0Texture;
SamplerState color0Sampler;

// xyz: World normals    w: Specular power
Texture2D color1Texture;
SamplerState color1Sampler;

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain(PS_INPUT input) : SV_TARGET
{
	float4 color0 = color0Texture.Sample(color0Sampler, input.vTexCoord0);
	return float4(color0.xyz, 1);
}

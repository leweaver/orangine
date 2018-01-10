//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
	float4 vColor       : COLOR0;
	float3 vNormal      : NORMAL;
	float4 vTangent     : TANGENT;
	float2 vTexCoord0   : TEXCOORD0;
};

Texture2D baseColorTexture;
SamplerState baseColorSampler;

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain(PS_INPUT input) : SV_TARGET
{
	return baseColorTexture.Sample(baseColorSampler, input.vTexCoord0);
}

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
	float4 vColor       : COLOR0;
	float3 vNormal      : NORMAL0;
	float3 vWorldNormal : NORMAL1;
	float4 vTangent     : TANGENT;
	float2 vTexCoord0   : TEXCOORD0;
};

struct PS_OUTPUT
{
	float4  Color  : SV_Target0; // xyz: Diffuse          w: Specular intensity
	float4  Color1 : SV_Target1; // xyz: World normals    w: Specular power
};

Texture2D baseColorTexture;
SamplerState baseColorSampler;

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
PS_OUTPUT PSMain(PS_INPUT input)
{
	PS_OUTPUT output;
	output.Color = baseColorTexture.Sample(baseColorSampler, input.vTexCoord0);
	output.Color1 = float4(input.vWorldNormal.xyz, 1.0);

	return output;
}

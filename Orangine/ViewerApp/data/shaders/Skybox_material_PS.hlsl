//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
TextureCube gCubeMap;
SamplerState gSampler;

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
	float4 vPosition    : SV_POSITION;
	float3 vPositionL   : POSITION;
};

struct PS_OUTPUT
{
	float4 color : SV_TARGET;
	float  depth : SV_DEPTH;
};

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
PS_OUTPUT PSMain(PS_INPUT input)
{
	PS_OUTPUT output;
	output.color = gCubeMap.Sample(gSampler, input.vPositionL);
	output.depth = 1.0f;
	return output;
}
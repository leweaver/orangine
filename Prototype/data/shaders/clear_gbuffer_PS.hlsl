//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
};

struct PS_OUTPUT
{
	float4 Color        : SV_Target0;
	float4 Normal       : SV_Target1;
};

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
PS_OUTPUT PSMain(PS_INPUT input)
{
	PS_OUTPUT output;    
	output.Color = 0.0f;
	output.Color.a = 1.0f;

	//when transforming 0.5f into [-1,1], we will get 0.0f
	output.Normal.rgb = 0.5f;

	//no specular power
	output.Normal.a = 0.0f;

	//max depth
	//output.Depth = 1.0f;

	return output;
}

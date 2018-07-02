//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
};

struct PS_OUTPUT
{
	float4 Color0        : SV_Target0;
	float4 Color1       : SV_Target1;
	float4 Color2       : SV_Target2;
};

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
PS_OUTPUT PSMain(PS_INPUT input)
{
	PS_OUTPUT output;    
	// TODO: Bring clear color in as a constant
	//output.Color0 = float4(0.78f, 0.64f, 0.78f, 1.0f);
	output.Color0 = float4(0.0f, 0.0f, 0.2f, 1.0f);

	//when transforming 0.5f into [-1,1], we will get 0.0f
	output.Color1 = float4(0.5f, 0.5f, 0.5f, 0.0f);

	output.Color2 = float4(0, 0, 0, 1);

	return output;
}

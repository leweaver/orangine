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
	float4 Color        : COLOR0;
	float4 Normal       : COLOR1;
	float4 Depth        : COLOR2;
};

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
PS_OUTPUT PSMain(PS_INPUT input) : SV_TARGET
{
	PS_OUTPUT output;    
	output.Color = 0.0f;
	output.Color.a = 0.0f;

	//when transforming 0.5f into [-1,1], we will get 0.0f
	output.Normal.rgb = 0.5f;

	//no specular power
	output.Normal.a = 0.0f;

	//max depth
	output.Depth = 1.0f;

	return output;
}

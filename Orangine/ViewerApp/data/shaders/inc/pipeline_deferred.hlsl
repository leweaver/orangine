struct PS_OUTPUT
{
	float4  Color  : SV_Target0; // rgb: Diffuse          a: metallic
	float4  Color1 : SV_Target1; // rgb: World normals    a: roughness
	float4  Color2 : SV_Target2; // rgb: Emissive         a: Occlusion
};

PS_OUTPUT EncodeOutput(
	float4 baseColor, 
	float metallic, 
	float3 worldNormal, 
	float roughness, 
	float3 emissive, 
	float occlusion,
	float3 worldPosition)
{
	PS_OUTPUT output;

	output.Color = float4(baseColor.rgb, metallic);
	output.Color1 = float4(worldNormal, roughness);
	output.Color2 = float4(emissive, occlusion);

	return output;
}
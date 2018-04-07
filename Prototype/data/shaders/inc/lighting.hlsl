
//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------

static const float3 g_dielectricSpecular = { 0.04, 0.04, 0.04 };
static const float3 g_black = { 0, 0, 0 };
static const float  g_PI = 3.14159;

//--------------------------------------------------------------------------------------
// Lighting Term Calculation Methods
//--------------------------------------------------------------------------------------

float3 surfaceReflectionRatio(float3 F0, float3 VdotH)
{
	return F0 + (1 - F0) * pow(1.0 - VdotH, 5);
}

// Returns a value between 0 and 1
float geometricOcclusion(float roughness, float3 LdotH, float3 NdotH)
{
	// roughness * sqrt(2 / PI)
	float k = roughness * 0.253974543736;
	float omk = 1 - k;

	float vL = LdotH / (LdotH * omk + k);
	float vN = NdotH / (NdotH * omk + k);

	return vL * vN;
}

// Returns a value between 0 and 1
float microfacedDistribution(float alphaSqr, float3 NdotH)
{
	float expr = ((NdotH * NdotH) * (alphaSqr - 1) + 1);
	return alphaSqr / (g_PI * expr * expr);
}

//--------------------------------------------------------------------------------------
// BRDF
// bidirectional reflectance distribution function
//--------------------------------------------------------------------------------------

float3 BRDF(in float metallic, in float roughness, in float3 baseColor, 
	in float3 eyeVectorW, in float3 lightVectorW, in float3 normalW)
{
	// BRDF inputs, as per glTF specification
	// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-b-brdf-implementation

	// color differential
	float3 c = lerp(baseColor * (1 - g_dielectricSpecular.r), g_black, metallic);
	// normal incidence
	float3 F0 = lerp(g_dielectricSpecular, baseColor, metallic);
	// alpha
	float alpha = roughness * roughness;
	float alphaSqr = alpha * alpha;
	
	float3 halfVectorW = normalize(eyeVectorW + lightVectorW);

	float invNdotL = clamp(-dot(normalW, lightVectorW), 0.0, 1.0);
	float NdotL = clamp(dot(normalW, lightVectorW), 0.001, 1.0);
	float NdotV = abs(dot(normalW, eyeVectorW)) + 0.001;
	float NdotH = clamp(dot(normalW, halfVectorW), 0.0, 1.0);
	float LdotH = clamp(dot(lightVectorW, halfVectorW), 0.0, 1.0);
	float VdotH = clamp(dot(eyeVectorW, halfVectorW), 0.0, 1.0);

	// BRDF Calculation (using Schlick BRDF model)
	// https://www.cs.virginia.edu/%7Ejdl/bib/appearance/analytic%20models/schlick94b.pdf

	// BDRF terms
	float3 F = surfaceReflectionRatio(F0, VdotH);
	float G = geometricOcclusion(roughness, LdotH, NdotH);
	float D = microfacedDistribution(alphaSqr, NdotH);
	float3 diffuse = c / g_PI;

	// BDRF Calculation
	float3 fDiffuse = (1 - F) * diffuse;
	float3 fSpecular = F * G * D / (4 * NdotL * NdotV);

	return invNdotL * (fDiffuse + fSpecular);
}
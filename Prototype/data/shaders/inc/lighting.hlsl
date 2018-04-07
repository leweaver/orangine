
//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------

static const float3 g_dielectricSpecular = { 0.04, 0.04, 0.04 };
static const float3 g_black = { 0, 0, 0 };
static const float  g_PI = 3.14159;

//--------------------------------------------------------------------------------------
// Lighting Term Calculation Methods
//--------------------------------------------------------------------------------------

float3 surfaceReflectionRatio(float3 F0, float3 eyeVector, float3 halfVector)
{
	return F0 + (1 - F0) * pow(1.0 - dot(eyeVector, halfVector), 5);
}

float3 geometricOcclusionInt(float3 value, float3 halfVector, float k)
{
	float dp = dot(value, halfVector);
	return dp / (dp * (1 - k) + k);
}

float3 geometricOcclusion(float roughness, float3 lightVector, float3 normal, float3 halfVector)
{
	// sqrt(2 / PI)
	float k = roughness * 0.253974543736;

	return geometricOcclusionInt(lightVector, halfVector, k) * geometricOcclusionInt(normal, halfVector, k);
}

float3 microfacedDistribution(float alphaSqr, float3 normal, float3 halfVector)
{
	float dp = dot(normal, halfVector);
	float expr = ((dp*dp) * (alphaSqr - 1) + 1);
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

	// BRDF Calculation (using Schlick BRDF model)
	// https://www.cs.virginia.edu/%7Ejdl/bib/appearance/analytic%20models/schlick94b.pdf

	// BDRF terms
	float3 F = surfaceReflectionRatio(F0, eyeVectorW, halfVectorW);
	float3 G = geometricOcclusion(roughness, lightVectorW, normalW, halfVectorW);
	float3 D = microfacedDistribution(alphaSqr, normalW, halfVectorW);
	float3 diffuse = c / g_PI;

	// BDRF Calculation
	float3 fDiffuse = (1 - F) * diffuse;
	float3 fSpecular = (F * G * D) /
		(4 * dot(normalW, lightVectorW) * dot(normalW, eyeVectorW));

	return fDiffuse + fSpecular;
}

//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_AMBIENT 2

static const float3 g_dielectricSpecular = { 0.04, 0.04, 0.04 };
static const float3 g_black = { 0, 0, 0 };
static const float  g_PI = 3.14159;

// For use in constant buffers
struct Light {
	uint     type;
	float3   directionPosition;
	float3   intensifiedColor;
	int      shadowMapIndex;
	float4x4 shadowMapViewMatrix;
	float    shadowMapDepth;
	float    shadowMapBias;
	float2   notused;
};

//--------------------------------------------------------------------------------------
// Lighting Term Calculation Methods
//--------------------------------------------------------------------------------------

float3 surfaceReflectionRatio(float3 F0, float3 VdotH)
{

	//return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
	return F0 + (1 - F0) * pow(clamp(1.0 - VdotH, 0, 1), 5);
}

// Returns a value between 0 and 1
float geometricOcclusion(float roughness, float NdotL, float NdotV)
{
	// glTF Reference Implementation:
	// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#geometric-occlusion-g		
	//float k = roughness * 0.253974543736;// roughness * sqrt(2 / PI)
	float k = ((roughness + 1) * (roughness + 1)) / 8; // reduces the hotness of high incidence surfaces.
	float omk = 1 - k;

	float vL = NdotL / (NdotL * omk + k);
	float vV = NdotV / (NdotV * omk + k);

	return vL * vV;

	// glTF Sample Implementation:
	// https://github.com/KhronosGroup/glTF-WebGL-PBR/blob/master/shaders/pbr-frag.glsl
	//
	// This implementation is based on [1] Equation 4, and we adopt their modifications to
	// alphaRoughness as input as originally proposed in [2].
	// [1] http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
	// [1] http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
	/*
	float r2 = roughness * roughness;
	float oneMinusR2 = 1.0 - r2;
	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r2 + oneMinusR2 * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r2 + oneMinusR2 * (NdotV * NdotV)));
	return attenuationL * attenuationV;
	*/
}

// Returns a value between 0 and 1
float microfacedDistribution(float alphaSqr, float NdotH)
{
	float expr = ((NdotH * NdotH) * (alphaSqr - 1) + 1);
	return alphaSqr / (g_PI * expr * expr);
}

//--------------------------------------------------------------------------------------
// BRDF
// bidirectional reflectance distribution function
//--------------------------------------------------------------------------------------
struct LightingInputs
{
	float NdotV;// = abs(dot(normalW, eyeVectorW)) + 0.001;
};
float3 BRDF(in LightingInputs inputs, in float metallic, in float roughness, in float3 baseColor,
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
	
	float NdotL = clamp(dot(normalW, lightVectorW), 0.001, 1.0);
	float NdotH = clamp(dot(normalW, halfVectorW), 0.0, 1.0);
	float LdotH = clamp(dot(lightVectorW, halfVectorW), 0.0, 1.0);
	float VdotH = LdotH;

	// BRDF Calculation (using Schlick BRDF model)
	// https://www.cs.virginia.edu/%7Ejdl/bib/appearance/analytic%20models/schlick94b.pdf

	// BDRF terms
	float3 F = surfaceReflectionRatio(F0, VdotH);
	float G = geometricOcclusion(roughness, NdotL, inputs.NdotV);
	float D = microfacedDistribution(alphaSqr, NdotH);
	float3 diffuse = c / g_PI;

	// BDRF Calculation
	float3 fDiffuse = (1 - F) * diffuse;
	float3 fSpecular = (F * G * D) / (4 * NdotL * inputs.NdotV);

	return NdotL * (fDiffuse + fSpecular);
}

// -----------------------
struct BRDFEnvInputs
{
	float3 surfaceNormal;
	float3 reflection;

	// PBR Common
	float roughness;
	float3 diffuseColor;
	float3 specularColor;

	// Constants
	float2 scaleIBLAmbient;
};
float3 BRDFEnv(BRDFEnvInputs inputs, LightingInputs li)
{
	float mipCount = 9.0; // resolution of 512x512
	float lod = (inputs.roughness * mipCount);

	// retrieve a scale and bias to F0. See [1], Figure 3
	// g_shadowMapDepthTexture.Sample(g_shadowMapSampler
	float3 brdf = 
		SRGBtoLINEAR(g_envBrdfLUTexture.Sample(g_envSampler, float2(li.NdotV, 1.0 - inputs.roughness)).rgb);
	float3 diffuseLight =
		SRGBtoLINEAR(g_envDiffuseTexture.Sample(g_envSampler, inputs.surfaceNormal).rgb);
	float3 specularLight =
		SRGBtoLINEAR(g_envSpecularTexture.Sample(g_envSampler, inputs.reflection, lod).rgb);

	float3 diffuse = diffuseLight * inputs.diffuseColor;
	float3 specular = specularLight * (inputs.specularColor * brdf.x + brdf.y);

	// For presentation, this allows us to disable IBL terms
	diffuse *= inputs.scaleIBLAmbient.x;
	specular *= inputs.scaleIBLAmbient.y;

	return diffuse + specular;
}

// -----------------------
struct BRDFLightInputs {
	int lightType;
	float3 lightIntensifiedColor;
	float3 lightPosition;

	float3 baseColor;
	float metallic;
	float roughness;
	float occlusion;

	float3 worldPosition;
	float3 worldNormal;
	float3 eyeVectorW;
};
float3 BRDFLight(BRDFLightInputs inputs, LightingInputs li)
{
	// Vector from surface point to light
	float3 lightVector;

	float3 finalColor;
	if (inputs.lightType == LIGHT_TYPE_AMBIENT)
	{
		finalColor = inputs.occlusion * inputs.lightIntensifiedColor * inputs.baseColor;
	}
	else
	{
		if (inputs.lightType == LIGHT_TYPE_POINT)
		{
			float3 posDifference = inputs.lightPosition - inputs.worldPosition;
			inputs.lightIntensifiedColor = inputs.lightIntensifiedColor / length(posDifference);
			lightVector = normalize(posDifference);
		}
		else if (inputs.lightType == LIGHT_TYPE_DIRECTIONAL)
		{
			lightVector = -inputs.lightPosition;
		}
		else
		{
			lightVector = float3(0, 0, 1);
		}

		// BRDF lighting calculations
		finalColor = inputs.lightIntensifiedColor * BRDF(
			li,
			inputs.metallic,
			inputs.roughness,
			inputs.baseColor,
			inputs.eyeVectorW,
			lightVector,
			inputs.worldNormal);
	}

	return finalColor;
}

// -----------------------
struct ShadowSampleInputs {
	float3 lightColor;
	float3 worldPosition;
	//TODO:
	//float2 viewportTopLeft;
	//float2 viewportSize;
	float  shadowMapArrayIndex;
	float4x4 shadowMapViewMatrix;

	float shadowMapDepth;
	float shadowMapBias;
};
float3 Shadow(ShadowSampleInputs ssi)
{
	float4 shadowMapPosition = mul(float4(ssi.worldPosition, 1), ssi.shadowMapViewMatrix);
	float3 shadowCoord = float3(shadowMapPosition.xy / shadowMapPosition.w, ssi.shadowMapArrayIndex);
	
	// Bring x,y from [-1, 1] to [0, 1]
	// TODO: Why do we need to y-flip here?
	shadowCoord = shadowCoord * float3(0.5, -0.5, 1) + float3(0.5, 0.5, 0);

	uint3 mapSize;
	g_shadowMapStencilTexture.GetDimensions(mapSize.x, mapSize.y, mapSize.z);

	const float depthSample = g_shadowMapDepthTexture.Sample(g_shadowMapSampler, shadowCoord.rgb).r;
	const uint stencilSample = g_shadowMapStencilTexture.Load(int4(
			shadowCoord.xy * mapSize.xy,
			ssi.shadowMapArrayIndex,
			0)
		).g;
	
	float3 outputColor = ssi.lightColor;
	if (stencilSample != 0) {
		float shadowSample = depthSample * ssi.shadowMapDepth + ssi.shadowMapBias;
		if (shadowMapPosition.z > shadowSample)
			outputColor = float3(0, 0, 0);
	}

	return outputColor;
}
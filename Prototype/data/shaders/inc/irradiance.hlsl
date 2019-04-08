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
    float mipCount = 10.0; // resolution of 512x512
    float lod = (inputs.roughness * mipCount);

    // retrieve a scale and bias to F0. See [1], Figure 3
    // g_shadowMapDepthTexture.Sample(g_shadowMapSampler
    float3 brdf =
        SRGBtoLINEAR(g_envBrdfLUTexture.Sample(g_envSampler, float2(li.NdotV, 1.0 - inputs.roughness)).rgb);

    // TODO: No idea why this line is throwing a warning:
    // warning X3206: 'SampleLevel': implicit truncation of vector type
    float3 diffuseLight =
        SRGBtoLINEAR(g_envDiffuseTexture.SampleLevel(g_envSampler, inputs.surfaceNormal.xyz, 0.0f).rgb);
    float3 specularLight =
        SRGBtoLINEAR(g_envSpecularTexture.SampleLevel(g_envSampler, inputs.reflection, lod).rgb);

    float3 diffuse = diffuseLight * inputs.diffuseColor;
    float3 specular = specularLight * (inputs.specularColor * brdf.x + brdf.y);

    // For presentation, this allows us to disable IBL terms
    diffuse *= inputs.scaleIBLAmbient.x;
    specular *= inputs.scaleIBLAmbient.y;

    return diffuse + specular;
}
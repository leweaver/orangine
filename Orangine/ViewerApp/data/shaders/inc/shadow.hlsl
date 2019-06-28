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
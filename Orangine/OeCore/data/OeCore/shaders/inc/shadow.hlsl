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
	int shadowMapDimension;
}; 

float3 Shadow(ShadowSampleInputs ssi)
{
    float4 shadowMapPosition = mul(ssi.shadowMapViewMatrix, float4(ssi.worldPosition, 1));
    float3 shadowCoord = float3(shadowMapPosition.xy / shadowMapPosition.w, ssi.shadowMapArrayIndex);

    // Bring x,y from [-1, 1] to [0, 1]
    // TODO: Why do we need to y-flip here?
    shadowCoord = shadowCoord * float3(0.5, -0.5, 1) + float3(0.5, 0.5, 0);

    const float depthSample = g_shadowMapDepthTexture.Sample(g_shadowMapSampler, shadowCoord.rgb).r;
    const uint stencilSample = g_shadowMapStencilTexture.Load(int4(
        shadowCoord.xy * ssi.shadowMapDimension,
        ssi.shadowMapArrayIndex,
        0)
    ).g;

    float3 outputColor = ssi.lightColor;
	float shadowSample = depthSample + ssi.shadowMapBias;
	
    /* logic of below formula:
	if (stencilSample != 0) {
        if (shadowMapPosition.z >= shadowSample)
            outputColor = float3(0, 0, 0);
    }*/
	float multiplier = max(1 - stencilSample, step(shadowMapPosition.z, shadowSample));

	return outputColor * multiplier;
}
#include "pch.h"

#include "OeCore/Renderer_data.h"
#include "OeCore/EngineUtils.h"

using namespace oe;


size_t Renderer_features_enabled::hash() const {
  size_t rendererFeaturesHash = 0;
  hash_combine(rendererFeaturesHash, skinnedAnimation);
  hash_combine(rendererFeaturesHash, vertexMorph);
  hash_combine(rendererFeaturesHash, debugDisplayMode);
  hash_combine(rendererFeaturesHash, shadowsEnabled);
  hash_combine(rendererFeaturesHash, irradianceMappingEnabled);
  hash_combine(rendererFeaturesHash, enableShaderOptimization);
  return rendererFeaturesHash;
}

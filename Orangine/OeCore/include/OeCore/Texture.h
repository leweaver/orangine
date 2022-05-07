#pragma once

#include "Renderer_types.h"

#include <json.hpp>

namespace oe {

enum class Texture_flags {
  Invalid = 0,
  Is_valid = 1 << 0,
  Is_array_texture = 1 << 1,// this is an array texture
  Is_cube_texture = 1 << 2,// this is a cube texture
  Is_array_slice = 1 << 3,  // is a single slice of an array texture
};

using TextureInternalId = int64_t;

class Texture {
 public:
  virtual ~Texture() = default;

  // All other fields are only guaranteed to return valid data if this method returns true.
  bool isValid() const
  {
    return _flags & static_cast<uint32_t>(Texture_flags::Is_valid);
  }

  const Sampler_descriptor& getSamplerDescriptor() const
  {
    return _samplerDescriptor;
  }

  Vector2u dimension() const
  {
    return _dimension;
  }
  bool isArraySlice() const
  {
    return _flags & static_cast<uint32_t>(Texture_flags::Is_array_slice);
  }
  bool isCubeMap() const
  {
    return _flags & static_cast<uint32_t>(Texture_flags::Is_cube_texture);
  }
  bool isArray() const
  {
    return _flags & static_cast<uint32_t>(Texture_flags::Is_array_texture);
  }

  // Returns which array slice this texture is; only valid when Is_array_slice flag is present.
  uint32_t arraySlice() const
  {
    return _arraySlice;
  }

  TextureInternalId internalId() const
  {
    return _internalId;
  }

  /**
   * A name that can be used to identify this texture.
   */
  virtual const std::string& getName() const = 0;

 protected:
  Texture(TextureInternalId internalId)
      : _samplerDescriptor(Sampler_descriptor())
      , _dimension({0, 0})
      , _flags(static_cast<uint32_t>(Texture_flags::Invalid))
      , _arraySlice(0)
      , _internalId(internalId)
  {}

  Texture(TextureInternalId internalId, Vector2u dimension, uint32_t flags)
      : _samplerDescriptor(Sampler_descriptor())
      , _dimension(dimension)
      , _flags(flags)
      , _arraySlice(0)
      , _internalId(internalId)
  {}

  Sampler_descriptor _samplerDescriptor;

  Vector2u _dimension;

  // mask of Texture_flags
  uint32_t _flags;

  uint32_t _arraySlice;

  TextureInternalId _internalId;
};

void to_json(nlohmann::json& j, const std::shared_ptr<Texture> texture);

void from_json(const nlohmann::json& j, std::shared_ptr<Texture>& texture);
}// namespace oe

#pragma once

#include <vectormath.hpp>

#include <json.hpp>

namespace Vectormath::SSE {

inline void to_json(nlohmann::json& j, const SSE::Vector4& vec4) {
  j = nlohmann::json{
      static_cast<float>(vec4.getX()),
      static_cast<float>(vec4.getY()),
      static_cast<float>(vec4.getZ()),
      static_cast<float>(vec4.getW())};
}

inline void from_json(const nlohmann::json& j, SSE::Vector4& vec4) {
  vec4.setX(j.at(0).get<float>());
  vec4.setY(j.at(1).get<float>());
  vec4.setZ(j.at(2).get<float>());
  vec4.setW(j.at(3).get<float>());
}
} // namespace Vectormath::SSE
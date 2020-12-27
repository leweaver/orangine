#pragma once

#include <vectormath/vectormath.hpp>

namespace oe {
class Color : public Vectormath::SSE::Vector4 {
 public:
  // ReSharper disable once CppNonExplicitConvertingConstructor
  Color(const SSE::Vector4& vec) : Vectormath::SSE::Vector4(vec) {}
  Color(const Color& other) : Vectormath::SSE::Vector4(other.get128()) {}

  Color(float r, float g, float b, float a = 1.0f) : Vectormath::SSE::Vector4(r, g, b, a) {}
  Color() : Vectormath::SSE::Vector4() {}
};

namespace Colors {
extern const Color White;
extern const Color Gray;
extern const Color Black;
extern const Color Red;
extern const Color Green;
extern const Color Blue;
extern const Color Transparent;
}; // namespace Colors
} // namespace oe

#pragma once

#include <vectormath/vectormath.hpp>

namespace oe {
namespace math {

constexpr float pi = 3.141592654f;
constexpr float two_pi = 6.283185307f;
constexpr float on_div_pi = 0.318309886f;    // 1 over PI
constexpr float one_div_2_pi = 0.159154943f; // 1 over 2*PI
constexpr float pi_div_2 = 1.570796327f;
constexpr float pi_div_4 = 0.785398163f;

extern const SSE::Vector3 left;
extern const SSE::Vector3 right;
extern const SSE::Vector3 up;
extern const SSE::Vector3 down;
extern const SSE::Vector3 forward;
extern const SSE::Vector3 backward;

} // namespace math
} // namespace oe
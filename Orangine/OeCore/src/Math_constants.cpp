#include "pch.h"

#include "OeCore/Math_constants.h"

const float oe::Math::PI = 3.141592654f;
const float oe::Math::PIMUL2 = 6.283185307f;
const float oe::Math::ONEDIVPI = 0.318309886f;
const float oe::Math::ONEDIV2PI = 0.159154943f;
const float oe::Math::PIDIV2 = 1.570796327f;
const float oe::Math::PIDIV4 = 0.785398163f;

const SSE::Vector3 oe::Math::Direction::Left = Vector3::xAxis() * -1.0f;
const SSE::Vector3 oe::Math::Direction::Right = Vector3::xAxis();
const SSE::Vector3 oe::Math::Direction::Up = Vector3::yAxis();
const SSE::Vector3 oe::Math::Direction::Down = Vector3::yAxis() * -1.0f;
const SSE::Vector3 oe::Math::Direction::Forward = Vector3::zAxis() * -1.0f;
const SSE::Vector3 oe::Math::Direction::Backward = Vector3::zAxis();
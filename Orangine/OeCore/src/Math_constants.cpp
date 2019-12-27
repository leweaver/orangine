#include "pch.h"

#include "OeCore/Math_constants.h"

const SSE::Vector3 oe::math::left = Vector3::xAxis() * -1.0f;
const SSE::Vector3 oe::math::right = Vector3::xAxis();
const SSE::Vector3 oe::math::up = Vector3::yAxis();
const SSE::Vector3 oe::math::down = Vector3::yAxis() * -1.0f;
const SSE::Vector3 oe::math::forward = Vector3::zAxis() * -1.0f;
const SSE::Vector3 oe::math::backward = Vector3::zAxis();
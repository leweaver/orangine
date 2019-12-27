#include "pch.h"

#include <OeCore/Camera_component.h>
#include <OeCore/EngineUtils.h>

using namespace oe;

DEFINE_COMPONENT_TYPE(Camera_component);

const float Camera_component::DEFAULT_FOV = degrees_to_radians(75.f);
const float Camera_component::DEFAULT_NEAR_PLANE = 0.1f;
const float Camera_component::DEFAULT_FAR_PLANE = 1000.0f;
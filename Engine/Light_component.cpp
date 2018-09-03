#include "pch.h"
#include "Light_component.h"

#include "Shadow_map_texture.h"

using namespace oe;

DEFINE_COMPONENT_TYPE(Directional_light_component);
DEFINE_COMPONENT_TYPE(Point_light_component);
DEFINE_COMPONENT_TYPE(Ambient_light_component);

Light_component::~Light_component()
{
	_shadowData.reset();
}
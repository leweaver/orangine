#include "OeCore/Light_component.h"
#include "OeCore/Shadow_map_texture.h"

using namespace oe;

DEFINE_COMPONENT_TYPE(Directional_light_component);
DEFINE_COMPONENT_TYPE(Point_light_component);
DEFINE_COMPONENT_TYPE(Ambient_light_component);

Light_component::~Light_component() { _shadowMapData.reset(); }
#include "pch.h"
#include "Light_provider.h"

using namespace oe;
using namespace DirectX;

Light_provider::Callback_type Light_provider::no_light_provider = [](const BoundingSphere&, std::vector<Entity*>&, uint32_t) {};

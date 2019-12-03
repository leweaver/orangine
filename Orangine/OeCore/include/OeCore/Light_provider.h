#pragma once

#include "Collision.h"

#include <functional>
#include <vector>

namespace oe {
	class Entity;

	class Light_provider {
	public:
		using Callback_type = std::function<void(const BoundingSphere& target, std::vector<Entity*>& lights, uint8_t maxLights)>;

		static Callback_type no_light_provider;
	};
}
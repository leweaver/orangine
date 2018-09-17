#pragma once

#include "../Engine/Entity_filter.h"

#include <vector>
#include <iterator>

namespace oe {
	class Entity;
}

namespace oe_test {
	class Fake_entity_filter : public oe::Entity_filter {
	public:
		explicit Fake_entity_filter(const std::vector<std::shared_ptr<oe::Entity>>& entities)
		{
			std::copy(entities.begin(), entities.end(), std::inserter(_entities, _entities.begin()));
		}

		explicit Fake_entity_filter(const std::set<std::shared_ptr<oe::Entity>>& entities)
		{
			_entities = entities;
		}
	};
}
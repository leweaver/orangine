#pragma once

#include <set>
#include <memory>

namespace oe {
	class Entity;

	class Entity_filter
	{
	public:

		using container = std::set<std::shared_ptr<Entity>>;
		using iterator = container::iterator;
		using const_iterator = container::const_iterator;

	protected:
		container _entities;

	public:
		Entity_filter() = default;
		virtual ~Entity_filter() = default;

		bool empty() const { return _entities.empty(); }

		iterator begin() { return _entities.begin(); }
		iterator end() { return _entities.end(); }

		const_iterator begin() const { return _entities.begin(); }
		const_iterator end() const { return _entities.end(); }

	};
}


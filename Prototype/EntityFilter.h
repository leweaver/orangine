#pragma once

#include <set>
#include <memory>

namespace OE {
	class Entity;

	class EntityFilter
	{
	public:

		using container = std::set<std::shared_ptr<Entity>>;
		using iterator = container::iterator;
		using const_iterator = container::const_iterator;

	protected:
		container m_entities;

	public:
		EntityFilter() = default;
		virtual ~EntityFilter() = default;

		bool empty() const { return m_entities.empty(); }

		iterator begin() { return m_entities.begin(); }
		iterator end() { return m_entities.end(); }

		const_iterator cbegin() const { return m_entities.begin(); }
		const_iterator cend() const { return m_entities.end(); }

	};
}


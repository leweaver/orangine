#pragma once

#include <set>
#include <memory>

namespace OE {
	class Entity;

	class EntityFilter
	{
	public:

		using container = typename std::set<std::shared_ptr<Entity>>;
		using iterator = typename container::iterator;
		using const_iterator = typename container::const_iterator;

	protected:
		container m_entities;

	public:
		EntityFilter() {}
		virtual ~EntityFilter() {}

		iterator begin() { return m_entities.begin(); }
		iterator end() { return m_entities.end(); }

		const_iterator cbegin() const { return m_entities.begin(); }
		const_iterator cend() const { return m_entities.end(); }

	};
}


#pragma once

namespace OE {
	class Entity;
	class Scene;

	/**
	 * Abstract base class for components, which are added to entities.
	 */
	class Component
	{
		Entity& m_entity;

	public:
		explicit Component(Entity& entity)
			: m_entity(entity)
		{
		}

		virtual ~Component() {}

		virtual void Initialize() = 0;
		virtual void Update() = 0;
	};

}

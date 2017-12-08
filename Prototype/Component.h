#pragma once

namespace OE {
	class Entity;
	class Scene;

	/**
	 * Abstract base class for components, which are added to entities.
	 */
	class Component
	{
	public:
		using ComponentType = unsigned int;

	private:
		static ComponentType ms_maxComponentId;

	public:
		/** Returns a new, unique component id */
		static ComponentType CreateComponentTypeId();

		Component() {}
		virtual ~Component() {}

		virtual ComponentType GetType() const = 0;

	};

}

#define DECLARE_COMPONENT_TYPE \
public:\
	ComponentType GetType() const override; \
	static ComponentType Type(); \
private:\
	static Component::ComponentType ms_typeId;

#define DEFINE_COMPONENT_TYPE(classname) \
Component::ComponentType classname::ms_typeId = Component::CreateComponentTypeId(); \
classname::ComponentType classname::GetType() const { return ms_typeId; }\
classname::ComponentType classname::Type() { return ms_typeId; }

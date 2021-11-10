#include "pch.h"
#include "OeCore/Entity_repository.h"
#include "OeCore/Entity.h"

using namespace oe;

Entity_repository::Entity_repository()
	: _lastEntityId(0)
{
}

std::shared_ptr<Entity> Entity_repository::instantiate(std::string_view name, IScene_graph_manager& sceneGraph, IComponent_factory& componentFactory)
{
	const auto entity = new Entity(sceneGraph, componentFactory, std::string(name), ++_lastEntityId);
	auto entityPtr = std::shared_ptr<Entity>(entity);
	_entities[_lastEntityId] = entityPtr;
	return entityPtr;
}

void Entity_repository::remove(const Entity::Id_type id)
{
	const auto pos = _entities.find(id);
	if (pos != _entities.end()) {
		_entities.erase(pos);
	}
}

std::shared_ptr<Entity> Entity_repository::getEntityPtrById(const Entity::Id_type id)
{
	const auto pos = _entities.find(id);
	if (pos == _entities.end())
		return nullptr;
	return pos->second;
}

Entity &Entity_repository::getEntityById(const Entity::Id_type id)
{
	const auto pos = _entities.find(id);
	assert(pos != _entities.end() && "Invalid Entity ID provided");
	return *pos->second.get();
}
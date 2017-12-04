#include "pch.h"
#include "EntityManager.h"

#include <algorithm>

using namespace OE;

EntityManager::EntityManager(Scene& scene)
	: m_scene(scene)
{
}

void EntityManager::Tick(double elapsedTime)
{
	if (!m_initialized)
	{
		for (auto const& weakPtr : m_rootEntities)
		{
			std::shared_ptr<Entity> entity = weakPtr.lock();
			if (entity == nullptr) {
				assert(false);
				continue;
			}

			entity->Initialize();
		}
		m_initialized = true;
	}

	m_deltaTime = elapsedTime;
	m_elapsedTime += elapsedTime;

	for (auto const& weakPtr : m_rootEntities)
	{
		std::shared_ptr<Entity> entity = weakPtr.lock();
		if (entity == nullptr) {
			assert(false);
			continue;
		}

		if (!entity->IsActive())
			continue;

		entity->Update();
	}
}

Entity& EntityManager::Instantiate(std::string name)
{
	Entity* entity = new Entity(this->m_scene, name, ++lastEntityId);
	const auto entityPtr = std::shared_ptr<Entity>(entity);
	m_entities[entity->GetId()] = entityPtr;
	m_rootEntities.push_back(std::weak_ptr<Entity>(entityPtr));

	if (m_initialized)
		entity->Initialize();

	return *entity;
}

Entity& EntityManager::Instantiate(std::string name, Entity& parent)
{
	Entity& entity = Instantiate(name);
	entity.SetParent(parent);
	return entity;
}

void EntityManager::Destroy(Entity& entity)
{
	entity.RemoveParent();
	RemoveFromRoot(entity);

	// This will delete the object. Make sure it is the last operation!
	m_entities.erase(entity.GetId());
}

void EntityManager::Destroy(const Entity::ID_TYPE& entityId)
{
	auto& entityPtr = m_entities[entityId];
	const auto entity = entityPtr.get();
	if (entity != nullptr) {
		Destroy(*entity);
	}
}

std::shared_ptr<Entity> EntityManager::RemoveFromRoot(const Entity& entity)
{
	const std::shared_ptr<Entity> entityPtr = m_entities[entity.GetId()];

	// remove from root array
	for (auto rootIter = m_rootEntities.begin(); rootIter != m_rootEntities.end(); ++rootIter) {
		const auto rootEntity = (*rootIter).lock();
		if (rootEntity != nullptr && rootEntity->GetId() == entity.GetId()) {
			m_rootEntities.erase(rootIter);
			break;
		}
	}

	return entityPtr;
}

void EntityManager::AddToRoot(const Entity& entity)
{
	m_rootEntities.push_back(m_entities[entity.GetId()]);
}


std::shared_ptr<EntityFilter> EntityManager::GetEntityFilter(const ComponentTypeSet &componentTypes) {
	// Does an entity filter exist with all of the given component types?
	for (auto efIter = m_entityFilters.begin(); efIter != m_entityFilters.end(); ++efIter) {
		if (std::equal(componentTypes.begin(), componentTypes.end(), (*efIter)->m_componentTypes.begin()))
			return *efIter;
	}

	auto filter = std::make_shared<EntityFilterImpl>(componentTypes.begin(), componentTypes.end());

	m_entityFilters.push_back(filter);
	return filter;
}

void EntityManager::HandleEntityAdd(const Entity &entity) {
	for (auto filter : m_entityFilters)
		filter->HandleEntityAdd(m_entities[entity.GetId()]);
}

void EntityManager::HandleEntityRemove(const Entity &entity) {
	for (auto filter : m_entityFilters)
		filter->HandleEntityRemove(m_entities[entity.GetId()]);
}

void EntityManager::HandleEntityComponentAdd(const Entity &entity, const Component &componentType) {
	for (auto filter : m_entityFilters)
		filter->HandleEntityComponentsUpdated(m_entities[entity.GetId()]);
}
void EntityManager::HandleEntityComponentRemove(const Entity &entity, const Component &componentType) {
	for (auto filter : m_entityFilters) {
		filter->HandleEntityComponentsUpdated(m_entities[entity.GetId()]);
	}
}

EntityManager::EntityFilterImpl::EntityFilterImpl(const ComponentTypeSet::const_iterator &begin, const ComponentTypeSet::const_iterator &end)
{
	m_componentTypes = std::set<Component::ComponentType>(begin, end);
}


void EntityManager::EntityFilterImpl::HandleEntityAdd(const std::shared_ptr<Entity> &entity)
{
	unsigned int foundCount = 0;

	// Add it to the filter if all of the components we look for is present.
	size_t componentCount = entity->GetComponentCount();
	for (size_t i = 0; i < componentCount; i++) {
		const Component &component = entity->GetComponent(i);

		auto pos = m_componentTypes.find(component.GetType());
		if (pos == m_componentTypes.end())
			break;

		++foundCount;
	}

	if (foundCount == static_cast<unsigned int>(componentCount))
		m_entities.insert(entity);
}

void EntityManager::EntityFilterImpl::HandleEntityRemove(const std::shared_ptr<Entity> &entity)
{
	// Remove it from the filter if any one of the components we look for is present.
	size_t componentCount = entity->GetComponentCount();
	for (size_t i = 0; i < componentCount; i++) {
		const Component &component = entity->GetComponent(i);
		
		auto pos = m_componentTypes.find(component.GetType());
		if (pos == m_componentTypes.end())
			continue;

		m_entities.erase(entity);
		break;
	}
}

void EntityManager::EntityFilterImpl::HandleEntityComponentsUpdated(const std::shared_ptr<Entity> &entity) {
	// TODO: Optimize.
	HandleEntityRemove(entity);
	HandleEntityAdd(entity);
}

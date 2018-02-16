#include "pch.h"
#include <algorithm>

#include "SceneGraphManager.h"
#include <deque>

using namespace OE;

SceneGraphManager::SceneGraphManager(Scene& scene, const std::shared_ptr<EntityRepository> &entityRepository)
	: ManagerBase(scene)
	, m_entityRepository(entityRepository)
{
}

SceneGraphManager::~SceneGraphManager()
{
	m_rootEntities.clear();
}

void SceneGraphManager::Initialize()
{
	assert(m_rootEntities.size() == 0);
}

void SceneGraphManager::Tick()
{
	if (!m_initialized)
	{
		for (auto const& entity : m_rootEntities)
		{
			if (entity == nullptr) {
				assert(false);
				continue;
			}

			InitializeEntity(entity);
		}
		m_initialized = true;
	}

	for (auto const& entity : m_rootEntities)
	{
		if (entity == nullptr) {
			assert(false);
			continue;
		}

		if (!entity->IsActive())
			continue;

		entity->Update();
	}
}

Entity& SceneGraphManager::Instantiate(const std::string &name)
{
	return Instantiate(name, nullptr);
}

Entity& SceneGraphManager::Instantiate(const std::string &name, Entity &parentEntity)
{
	auto entity = m_entityRepository->GetEntityPtrById(parentEntity.GetId());
	return Instantiate(name, entity.get());
}

Entity& SceneGraphManager::Instantiate(const std::string &name, Entity *parentEntity)
{
	const auto entityPtr = m_entityRepository->Instantiate(name);
	m_rootEntities.push_back(entityPtr);

	if (m_initialized)
		InitializeEntity(entityPtr);
	
	if (parentEntity)
		entityPtr->SetParent(*parentEntity);

	AddEntityToScene(entityPtr);

	return *entityPtr;
}

void SceneGraphManager::InitializeEntity(const std::shared_ptr<Entity>& entityPtr) const
{
	std::deque<Entity*> entities;
	entities.push_back(entityPtr.get());

	while (!entities.empty())
	{
		Entity *entity = entities.front();
		entities.pop_front();

		entity->ComputeWorldTransform();
		entity->m_state = EntityState::INITIALIZED;

		for (const auto &child : entity->m_children)
		{
			entities.push_back(child.get());
		}
	}
}

void SceneGraphManager::AddEntityToScene(const std::shared_ptr<Entity>& entityPtr) const
{
	entityPtr->m_state = EntityState::READY;
	m_scene.OnEntityAdded(*entityPtr.get());
}

void SceneGraphManager::Destroy(Entity::ID_TYPE entityId)
{
	auto entityPtr = m_entityRepository->GetEntityPtrById(entityId);
	if (entityPtr != nullptr) {

		entityPtr->m_state = EntityState::DESTROYED;

		entityPtr->RemoveParent();

		RemoveFromRoot(entityPtr);

		// This will delete the object. Make sure it is the last operation!
		m_entityRepository->Remove(entityId);
	}
}

std::shared_ptr<Entity> SceneGraphManager::GetEntityPtrById(Entity::ID_TYPE id) const
{
	return m_entityRepository->GetEntityPtrById(id);
}

Entity &SceneGraphManager::GetEntityById(const Entity::ID_TYPE id) const
{
	return m_entityRepository->GetEntityById(id);
}

std::shared_ptr<Entity> SceneGraphManager::RemoveFromRoot(std::shared_ptr<Entity> entityPtr)
{
	// remove from root array
	for (auto rootIter = m_rootEntities.begin(); rootIter != m_rootEntities.end(); ++rootIter) {
		const auto rootEntity = (*rootIter);
		if (rootEntity != nullptr && rootEntity->GetId() == entityPtr->GetId()) {
			m_rootEntities.erase(rootIter);
			break;
		}
	}

	return entityPtr;
}

void SceneGraphManager::AddToRoot(std::shared_ptr<Entity> entityPtr)
{
	m_rootEntities.push_back(entityPtr);
}

std::shared_ptr<EntityFilter> SceneGraphManager::GetEntityFilter(const ComponentTypeSet &componentTypes) {
	// Does an entity filter exist with all of the given component types?
	for (auto efIter = m_entityFilters.begin(); efIter != m_entityFilters.end(); ++efIter) {
		if (std::equal(componentTypes.begin(), componentTypes.end(), (*efIter)->m_componentTypes.begin()))
			return *efIter;
	}

	auto filter = std::make_shared<EntityFilterImpl>(componentTypes.begin(), componentTypes.end());

	m_entityFilters.push_back(filter);
	return filter;
}

void SceneGraphManager::HandleEntityAdd(const Entity &entity) {
	const auto entityPtr = m_entityRepository->GetEntityPtrById(entity.GetId());
	if (entityPtr) {
		for (auto filter : m_entityFilters)
			filter->HandleEntityAdd(entityPtr);
	}
}

void SceneGraphManager::HandleEntityRemove(const Entity &entity) {
	const auto entityPtr = m_entityRepository->GetEntityPtrById(entity.GetId());
	if (entityPtr) {
		for (auto filter : m_entityFilters)
			filter->HandleEntityRemove(entityPtr);
	}
}

void SceneGraphManager::HandleEntityComponentAdd(const Entity &entity, const Component &componentType) {
	const auto entityPtr = m_entityRepository->GetEntityPtrById(entity.GetId());
	if (entityPtr) {
		for (auto filter : m_entityFilters)
			filter->HandleEntityComponentsUpdated(entityPtr);
	}
}
void SceneGraphManager::HandleEntityComponentRemove(const Entity &entity, const Component &componentType) {
	const auto entityPtr = m_entityRepository->GetEntityPtrById(entity.GetId());
	if (entityPtr) {
		for (auto filter : m_entityFilters) {
			filter->HandleEntityComponentsUpdated(entityPtr);
		}
	}
}

void SceneGraphManager::HandleEntitiesLoaded(const std::vector<std::shared_ptr<Entity>> &loadedEntities)
{
	for (const auto &entity : loadedEntities)
	{
		if (!entity->HasParent())
		{
			m_rootEntities.push_back(entity);
		}
	}

	std::deque<std::shared_ptr<Entity>> newEntities(loadedEntities.begin(), loadedEntities.end());
	while (!newEntities.empty())
	{
		std::shared_ptr<Entity> entityPtr = newEntities.front();
		newEntities.pop_front();

		assert(entityPtr != nullptr);
		AddEntityToScene(entityPtr);

		newEntities.insert(newEntities.end(), entityPtr->m_children.begin(), entityPtr->m_children.end());
	}

	if (m_initialized)
	{
		for (const auto &entity : loadedEntities)
			InitializeEntity(entity);
	}
}

SceneGraphManager::EntityFilterImpl::EntityFilterImpl(const ComponentTypeSet::const_iterator &begin, const ComponentTypeSet::const_iterator &end)
{
	m_componentTypes = std::set<Component::ComponentType>(begin, end);
}


void SceneGraphManager::EntityFilterImpl::HandleEntityAdd(const std::shared_ptr<Entity> &entity)
{
	size_t foundCount = 0;

	// Add it to the filter if all of the components we look for is present.
	size_t componentCount = entity->GetComponentCount();
	for (size_t i = 0; i < componentCount; i++) {
		const Component &component = entity->GetComponent(i);

		auto pos = m_componentTypes.find(component.GetType());
		if (pos == m_componentTypes.end())
			continue;

		++foundCount;
	}

	if (foundCount == m_componentTypes.size())
		m_entities.insert(entity);
}

void SceneGraphManager::EntityFilterImpl::HandleEntityRemove(const std::shared_ptr<Entity> &entity)
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

void SceneGraphManager::EntityFilterImpl::HandleEntityComponentsUpdated(const std::shared_ptr<Entity> &entity) {
	// TODO: Optimize.
	HandleEntityRemove(entity);
	HandleEntityAdd(entity);
}

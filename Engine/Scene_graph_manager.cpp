#include "pch.h"

#include "Scene_graph_manager.h"
#include "Scene.h"

#include <algorithm>
#include <deque>
#include "imgui.h"

using namespace oe;
using namespace internal;

template<>
IScene_graph_manager* oe::create_manager(Scene & scene, std::shared_ptr<IEntity_repository>& entityRepository)
{
    return new Scene_graph_manager(scene, entityRepository);
}

Scene_graph_manager::Scene_graph_manager(Scene& scene, std::shared_ptr<IEntity_repository> entityRepository)
	: IScene_graph_manager(scene)
	, _entityRepository(std::move(entityRepository))
{
}

void Scene_graph_manager::initialize()
{
    assert(_rootEntities.empty());
}

void Scene_graph_manager::shutdown()
{
}

void Scene_graph_manager::tick()
{
    if (!_initialized)
    {
        for (auto const& entity : _rootEntities)
        {
            if (entity == nullptr) {
                assert(false);
                continue;
            }

            initializeEntity(entity);
        }
        _initialized = true;
    }

    for (auto const& entity : _rootEntities)
    {
        const auto entityPtr = entity.get();
        if (entityPtr == nullptr) {
            assert(false);
            continue;
        }

        if (!entityPtr->isActive())
            continue;

        updateEntity(entityPtr);
    }
}

void Scene_graph_manager::updateEntity(Entity* entity)
{
    entity->computeWorldTransform();

    if (entity->hasChildren()) {
        const auto& children = entity->children();
        for (auto const& child : children) {
            if (child->isActive())
                updateEntity(child.get());
        }

        if (entity->calculateBoundSphereFromChildren()) {

            auto accumulatedBounds = children[0]->boundSphere();
            for (auto pos = children.begin() + 1; pos < children.end(); ++pos) {
                DirectX::BoundingSphere::CreateMerged(accumulatedBounds, accumulatedBounds, (*pos)->boundSphere());
            }

            entity->setBoundSphere(accumulatedBounds);
        }
    }
}

std::shared_ptr<Entity> Scene_graph_manager::instantiate(const std::string &name)
{
    return instantiate(name, nullptr);
}

std::shared_ptr<Entity> Scene_graph_manager::instantiate(const std::string &name, Entity &parentEntity)
{
    auto entity = _entityRepository->getEntityPtrById(parentEntity.getId());
    return instantiate(name, entity.get());
}

void Scene_graph_manager::renderImGui()
{
//    ImGui::Begin("Entities");

//    ImGui::End();
}

std::shared_ptr<Entity> Scene_graph_manager::instantiate(const std::string &name, Entity *parentEntity)
{
    const auto entityPtr = _entityRepository->instantiate(name);
    _rootEntities.push_back(entityPtr);

    if (_initialized)
        initializeEntity(entityPtr);

    if (parentEntity)
        entityPtr->setParent(*parentEntity);

    addEntityToScene(entityPtr);

    return entityPtr;
}

void Scene_graph_manager::initializeEntity(std::shared_ptr<Entity> entityPtr) const
{
    std::deque<Entity*> entities;
    entities.push_back(entityPtr.get());

    while (!entities.empty())
    {
        auto entity = entities.front();
        entities.pop_front();

        entity->computeWorldTransform();
        entity->_state = Entity_state::Initialized;

        for (const auto &child : entity->_children)
        {
            entities.push_back(child.get());
        }
    }
}

void Scene_graph_manager::addEntityToScene(std::shared_ptr<Entity> entityPtr) const
{
    entityPtr->_state = Entity_state::Ready;
    _scene.onEntityAdded(*entityPtr.get());
}

void Scene_graph_manager::destroy(Entity::Id_type entityId)
{
    auto entityPtr = _entityRepository->getEntityPtrById(entityId);
    if (entityPtr != nullptr) {

        entityPtr->_state = Entity_state::Destroyed;

        entityPtr->removeParent();

        removeFromRoot(entityPtr);

        // This will delete the object. Make sure it is the last operation!
        _entityRepository->remove(entityId);
    }
}

std::shared_ptr<Entity> Scene_graph_manager::getEntityPtrById(Entity::Id_type id) const
{
    return _entityRepository->getEntityPtrById(id);
}

std::shared_ptr<Entity> Scene_graph_manager::removeFromRoot(std::shared_ptr<Entity> entityPtr)
{
    // remove from root array
    for (auto rootIter = _rootEntities.begin(); rootIter != _rootEntities.end(); ++rootIter) {
        const auto rootEntity = (*rootIter);
        if (rootEntity != nullptr && rootEntity->getId() == entityPtr->getId()) {
            _rootEntities.erase(rootIter);
            break;
        }
    }

    return entityPtr;
}

void Scene_graph_manager::addToRoot(std::shared_ptr<Entity> entityPtr)
{
    _rootEntities.push_back(entityPtr);
}

std::shared_ptr<Entity_filter> Scene_graph_manager::getEntityFilter(const Component_type_set &componentTypes, Entity_filter_mode mode) {
    // Does an entity filter exist with all of the given component types?
    for (auto efIter = m_entityFilters.begin(); efIter != m_entityFilters.end(); ++efIter) {
        const auto &ef = *efIter;
        if (ef->mode != mode)
            continue;

        if (std::equal(componentTypes.begin(), componentTypes.end(), ef->componentTypes.begin()))
            return *efIter;
    }

    auto filter = std::make_shared<Entity_filter_impl>(componentTypes.begin(), componentTypes.end(), mode);

    m_entityFilters.push_back(filter);
    return filter;
}

void Scene_graph_manager::handleEntityAdd(const Entity &entity) {
    const auto entityPtr = _entityRepository->getEntityPtrById(entity.getId());
    if (entityPtr) {
        for (const auto& filter : m_entityFilters)
            filter->handleEntityAdd(entityPtr);
    }
}

void Scene_graph_manager::handleEntityRemove(const Entity &entity) {
    const auto entityPtr = _entityRepository->getEntityPtrById(entity.getId());
    if (entityPtr) {
        for (const auto& filter : m_entityFilters)
            filter->handleEntityRemove(entityPtr);
    }
}

void Scene_graph_manager::handleEntityComponentAdd(const Entity &entity, const Component &componentType) {
    const auto entityPtr = _entityRepository->getEntityPtrById(entity.getId());
    if (entityPtr) {
        for (const auto& filter : m_entityFilters)
            filter->handleEntityComponentsUpdated(entityPtr);
    }
}
void Scene_graph_manager::handleEntityComponentRemove(const Entity &entity, const Component &componentType) {
    const auto entityPtr = _entityRepository->getEntityPtrById(entity.getId());
    if (entityPtr) {
        for (const auto& filter : m_entityFilters) {
            filter->handleEntityComponentsUpdated(entityPtr);
        }
    }
}

void Scene_graph_manager::handleEntitiesLoaded(const std::vector<std::shared_ptr<Entity>> &loadedEntities)
{
    for (const auto &entity : loadedEntities)
    {
        if (!entity->hasParent())
        {
            _rootEntities.push_back(entity);
        }
    }

    std::deque<std::shared_ptr<Entity>> newEntities(loadedEntities.begin(), loadedEntities.end());
    while (!newEntities.empty())
    {
        auto entityPtr = newEntities.front();
        newEntities.pop_front();

        assert(entityPtr != nullptr);
        addEntityToScene(entityPtr);

        newEntities.insert(newEntities.end(), entityPtr->_children.begin(), entityPtr->_children.end());
    }

    if (_initialized)
    {
        for (const auto &entity : loadedEntities)
            initializeEntity(entity);
    }
}

Scene_graph_manager::Entity_filter_impl::Entity_filter_impl(const Component_type_set::const_iterator& begin,
    const Component_type_set::const_iterator& end,
    Entity_filter_mode mode)
    : mode(mode)
{
    componentTypes = std::set<Component::Component_type>(begin, end);
}


void Scene_graph_manager::Entity_filter_impl::handleEntityAdd(const std::shared_ptr<Entity>& entity)
{
    // Add it to the filter if all of the components we look for is present.
    const auto componentCount = entity->getComponentCount();

    if (mode == Entity_filter_mode::All) {
        // An entity can contain multiple instances of a single component type.
        Component_type_set foundComponents;
        for (size_t i = 0; i < componentCount; i++) {
            const auto& component = entity->getComponent(i);

            const auto pos = componentTypes.find(component.getType());
            if (pos == componentTypes.end())
                continue;

            foundComponents.insert(component.getType());
        }

        if (foundComponents.size() == componentTypes.size())
            _entities.insert(entity);
    }
    else
    {
        assert(mode == Entity_filter_mode::Any);
        for (size_t i = 0; i < componentCount; i++) {
            const auto& component = entity->getComponent(i);

            const auto pos = componentTypes.find(component.getType());
            if (pos != componentTypes.end())
            {
                _entities.insert(entity);
                return;
            }
        }
    }
}

void Scene_graph_manager::Entity_filter_impl::handleEntityRemove(const std::shared_ptr<Entity>& entity)
{
    // Remove it from the filter if any one of the components we look for is present (early exit).
    const auto componentCount = entity->getComponentCount();
    for (size_t i = 0; i < componentCount; i++) {
        const auto& component = entity->getComponent(i);

        const auto pos = componentTypes.find(component.getType());
        if (pos == componentTypes.end())
            continue;

        _entities.erase(entity);
        break;
    }
}

void Scene_graph_manager::Entity_filter_impl::handleEntityComponentsUpdated(const std::shared_ptr<Entity>& entity) {
	// TODO: Optimize.
	handleEntityRemove(entity);
	handleEntityAdd(entity);
}

#include "Scene_graph_manager.h"

#include <imgui.h>

#include <OeCore/EngineUtils.h>
#include <algorithm>
#include <deque>

using namespace oe;
using namespace internal;

std::string Scene_graph_manager::_name = "Scene_graph_manager";

template<>
void oe::create_manager(
        Manager_instance<IScene_graph_manager>& out, std::shared_ptr<IEntity_repository>& entityRepository)
{
  out = Manager_instance<IScene_graph_manager>(std::make_unique<Scene_graph_manager>(entityRepository));
}

Scene_graph_manager::Scene_graph_manager(std::shared_ptr<IEntity_repository> entityRepository)
    : IScene_graph_manager()
    , Manager_base()
    , Manager_tickable()
    , _entityRepository(std::move(entityRepository))
{}

void Scene_graph_manager::initialize() { assert(_rootEntities.empty()); }

void Scene_graph_manager::shutdown() {}

const std::string& Scene_graph_manager::name() const { return _name; }

void Scene_graph_manager::tick() {
  if (!_initialized) {
    for (auto const& entity : _rootEntities) {
      if (entity == nullptr) {
        assert(false);
        continue;
      }

      initializeEntity(entity);
    }
    _initialized = true;
  }

  for (auto const& entity : _rootEntities) {
    const auto entityPtr = entity.get();
    if (entityPtr == nullptr) {
      assert(false);
      continue;
    }

    if (!entityPtr->isActive()) {
      continue;
    }

    updateEntity(entityPtr);
  }
}

void Scene_graph_manager::updateEntity(Entity* entity) {
  if (entity->calculateWorldTransform()) {
    entity->computeWorldTransform();
  }

  if (entity->hasChildren()) {
    const auto& children = entity->children();
    for (auto const& child : children) {
      if (child->isActive()) {
        updateEntity(child.get());
      }
    }

    if (entity->calculateBoundSphereFromChildren()) {

      auto accumulatedBounds = children[0]->boundSphere();
      for (auto pos = children.begin() + 1; pos < children.end(); ++pos) {
        BoundingSphere::createMerged(accumulatedBounds, accumulatedBounds, (*pos)->boundSphere());
      }

      entity->setBoundSphere(accumulatedBounds);
    }
  }
}

std::shared_ptr<Entity> Scene_graph_manager::clone(const Entity& srcEntity, Entity* newParent) {
  auto ent = instantiate(srcEntity.getName(), newParent);
  for (const auto& srcChild : srcEntity.children()) {
    auto& child = *clone(*srcChild, ent.get());

    // Now copy components, and their values.
    const auto componentCount = srcChild->getComponentCount();
    for (auto i = 0; i < componentCount; ++i) {
      child.addCloneOfComponent(srcChild->getComponent(i));
    }
  }
  return ent;
}

std::shared_ptr<Entity> Scene_graph_manager::instantiate(const std::string& name) {
  return instantiate(name, nullptr);
}

std::shared_ptr<Entity> Scene_graph_manager::instantiate(
    const std::string& name,
    Entity& parentEntity) {
  auto entity = _entityRepository->getEntityPtrById(parentEntity.getId());
  return instantiate(name, entity.get());
}

void Scene_graph_manager::renderImGui() {
  //    ImGui::Begin("Entities");

  //    ImGui::End();
}

std::shared_ptr<Entity> Scene_graph_manager::instantiate(
    const std::string& name,
    Entity* parentEntity) {
  const auto entityPtr = _entityRepository->instantiate(name, *this, *this);
  _rootEntities.push_back(entityPtr);

  if (_initialized) {
    initializeEntity(entityPtr);
  }

  if (parentEntity) {
    entityPtr->setParent(*parentEntity);
  }

  addEntityToScene(entityPtr);

  return entityPtr;
}

void Scene_graph_manager::addLoader(std::unique_ptr<Entity_graph_loader> loader)
{
  std::vector<std::string> extensions;
  loader->getSupportedFileExtensions(extensions);
  for (const auto& extension : extensions) {
    if (_extensionToEntityGraphLoader.find(extension) != _extensionToEntityGraphLoader.end()) {
      OE_THROW(std::runtime_error(
              "Failed to register entity graph loader, file extension is already registered: " +
              extension));
    }
    _extensionToEntityGraphLoader[extension] = loader.get();
  }
  _entityGraphLoaders.push_back(std::move(loader));
}

void Scene_graph_manager::loadFile(const std::string& filename) {
  loadFile(filename, nullptr);
}

void Scene_graph_manager::loadFile(const std::string& filename, Entity* parentEntity) {
  // Get the file extension
  const auto dotPos = filename.find_last_of('.');
  if (dotPos == std::string::npos) {
    OE_THROW(std::runtime_error("Cannot load mesh; given file doesn't have an extension."));
  }

  const std::string extension = filename.substr(dotPos + 1);
  const auto extPos = _extensionToEntityGraphLoader.find(extension);
  if (extPos == _extensionToEntityGraphLoader.end()) {
    OE_THROW(std::runtime_error("Cannot load mesh; no registered loader for extension: " + extension));
  }

  std::vector<std::shared_ptr<Entity>> newRootEntities = extPos->second->loadFile(
          filename, *this, *_entityRepository, *this, true);

  if (parentEntity) {
    for (const auto& entity : newRootEntities) {
      entity->setParent(*parentEntity);
    }
  }

  handleEntitiesLoaded(newRootEntities);
}

void Scene_graph_manager::initializeEntity(std::shared_ptr<Entity> entityPtr) const {
  std::deque<Entity*> entities;
  entities.push_back(entityPtr.get());

  while (!entities.empty()) {
    auto entity = entities.front();
    entities.pop_front();

    if (entity->calculateWorldTransform()) {
      entity->computeWorldTransform();
    }

    entity->_state = Entity_state::Initialized;

    for (const auto& child : entity->_children) {
      entities.push_back(child.get());
    }
  }
}

void Scene_graph_manager::addEntityToScene(std::shared_ptr<Entity> entityPtr) {
  entityPtr->_state = Entity_state::Ready;

  onEntityAdd(*entityPtr);
}

void Scene_graph_manager::destroy(Entity::Id_type entityId) {
  auto entityPtr = _entityRepository->getEntityPtrById(entityId);
  if (entityPtr != nullptr) {

    entityPtr->_state = Entity_state::Destroyed;

    entityPtr->removeParent();

    removeFromRoot(entityPtr);

    onEntityRemove(*entityPtr);

    // This will delete the object. Make sure it is the last operation!
    _entityRepository->remove(entityId);
  }
}

std::shared_ptr<Entity> Scene_graph_manager::getEntityPtrById(Entity::Id_type id) const {
  return _entityRepository->getEntityPtrById(id);
}

std::shared_ptr<Entity> Scene_graph_manager::removeFromRoot(std::shared_ptr<Entity> entityPtr) {
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

void Scene_graph_manager::addToRoot(std::shared_ptr<Entity> entityPtr) {
  _rootEntities.push_back(entityPtr);
}

std::shared_ptr<Entity_filter> Scene_graph_manager::getEntityFilter(
    const Component_type_set& componentTypes,
    Entity_filter_mode mode) {
  // Does an entity filter exist with all of the given component types?
  for (auto efIter = m_entityFilters.begin(); efIter != m_entityFilters.end(); ++efIter) {
    const auto& ef = *efIter;
    if (ef->mode != mode) {
      continue;
    }

    if (std::equal(componentTypes.begin(), componentTypes.end(), ef->componentTypes.begin())) {
      return *efIter;
    }
  }

  auto filter = std::make_shared<Entity_filter_impl>(componentTypes.begin(), componentTypes.end(), mode);

  m_entityFilters.push_back(filter);
  return filter;
}

void Scene_graph_manager::onEntityAdd(const Entity& entity) {
  const auto entityPtr = _entityRepository->getEntityPtrById(entity.getId());
  if (entityPtr) {
    for (const auto& filter : m_entityFilters) {
      filter->handleEntityAdd(entityPtr);
    }
    _entityAddedDispatcher.invoke(*entityPtr);
  }
}

void Scene_graph_manager::onEntityRemove(const Entity& entity) {
  const auto entityPtr = _entityRepository->getEntityPtrById(entity.getId());
  if (entityPtr) {
    for (const auto& filter : m_entityFilters) {
      filter->handleEntityRemove(entityPtr);
    }
  }
}

void Scene_graph_manager::onEntityComponentAdd(
    const Entity& entity,
    const Component& componentType) {
  const auto entityPtr = _entityRepository->getEntityPtrById(entity.getId());
  if (entityPtr) {
    for (const auto& filter : m_entityFilters) {
      filter->handleEntityComponentsUpdated(entityPtr);
    }
  }
}
void Scene_graph_manager::onEntityComponentRemove(
    const Entity& entity,
    const Component& componentType) {
  const auto entityPtr = _entityRepository->getEntityPtrById(entity.getId());
  if (entityPtr) {
    for (const auto& filter : m_entityFilters) {
      filter->handleEntityComponentsUpdated(entityPtr);
    }
  }
}

void Scene_graph_manager::handleEntitiesLoaded(
    const std::vector<std::shared_ptr<Entity>>& loadedEntities) {
  for (const auto& entity : loadedEntities) {
    if (!entity->hasParent()) {
      _rootEntities.push_back(entity);
    }
  }

  std::deque<std::shared_ptr<Entity>> newEntities(loadedEntities.begin(), loadedEntities.end());
  while (!newEntities.empty()) {
    auto entityPtr = newEntities.front();
    newEntities.pop_front();

    assert(entityPtr != nullptr);
    addEntityToScene(entityPtr);

    newEntities.insert(newEntities.end(), entityPtr->_children.begin(), entityPtr->_children.end());
  }

  if (_initialized) {
    for (const auto& entity : loadedEntities) {
      initializeEntity(entity);
    }
  }
}

bool Scene_graph_manager::findCollidingEntity(
    const Ray& ray,
    Entity const*& foundEntity,
    Ray_intersection& foundIntersection,
    float maxDistance) {

  // Assume that the output arguments are garbage memory; init them.
  foundEntity = nullptr;
  foundIntersection = {{}, FLT_MAX};

  std::vector<const Entity*> work;
  std::transform(
      _rootEntities.begin(),
      _rootEntities.end(),
      std::back_inserter(work),
      [](const std::shared_ptr<Entity>& ent) { return ent.get(); });

  Ray_intersection intersection;

  while (!work.empty()) {
    const auto ent = work.back();
    work.pop_back();

    if (ent->boundSphere().radius <= 0.0f ||
        !intersect_ray_sphere(ray, ent->boundSphere(), intersection)) {
      continue;
    }

    if (intersection.distance >= foundIntersection.distance && foundEntity != ent->parent().get()) {
      // If the previous best was our parent, but we still collide; replace with this node even if it is farther away.

      if (intersection.distance >= maxDistance) {
        continue;
      }
    }

    foundIntersection = intersection;
    foundEntity = ent;

    std::transform(
        ent->children().begin(),
        ent->children().end(),
        std::back_inserter(work),
        [](const std::shared_ptr<Entity>& ent) { return ent.get(); });
  }

  return foundEntity != nullptr;
}
Component& Scene_graph_manager::addComponentToEntity(Component::Component_type typeId, Entity& entity)
{
  auto component = Component_factory::createComponent(typeId, entity);

  auto ptr = component.get();
  entity._components.push_back(std::move(component));
  onEntityComponentAdd(entity, *ptr);

  return *ptr;
}

Component& Scene_graph_manager::cloneComponentToEntity(const Component& srcComponent, Entity& entity)
{
  auto component = srcComponent.clone(entity);

  auto ptr = component.get();
  entity._components.push_back(std::move(component));
  onEntityComponentAdd(entity, *ptr);

  return *ptr;
}
void Scene_graph_manager::destroyComponent(Component& component)
{
  auto& entity = component.getEntity();
  onEntityComponentRemove(entity, component);
  for (auto pos = entity._components.begin(); pos != entity._components.end(); ++pos) {
    if (pos->get() == &component) {
      entity._components.erase(pos);
      break;
    }
  }
}

#include <OeCore/EngineUtils.h>

#include "OeCore/Entity.h"
#include "OeCore/Math_constants.h"
#include "Scene_graph_manager.h"

using namespace oe;
using namespace DirectX;

Entity::Entity(IScene_graph_manager& sceneGraph, IComponent_factory& componentFactory, std::string name, Id_type id)
    : _localRotation(SSE::Quat::identity())
    , _localPosition(Vector3(0, 0, 0))
    , _localScale(Vector3(1.0f, 1.0f, 1.0f))
    , _calculateWorldTransform(true)
    , _calculateBoundSphereFromChildren(true)
    , _id(id)
    , _name(std::move(name))
    , _state(Entity_state::Uninitialized)
    , _active(true)
    , _parent(nullptr)
    , _sceneGraph(sceneGraph)
    , _worldTransform(SSE::Matrix4::identity())
    , _worldRotation(SSE::Quat::identity())
    , _worldScale(Vector3(1.0f, 1.0f, 1.0f))
    , _componentFactory(componentFactory)
{}

void Entity::computeWorldTransform() {
  SSE::Matrix4 t;
  if (hasParent()) {
    _worldScale = mulPerElem(_parent->_worldScale, _localScale);
    _worldRotation = _parent->worldRotation() * _localRotation;

    const auto worldPosition = _parent->_worldTransform * SSE::Point3(_localPosition);
    t = SSE::Matrix4::translation(worldPosition.getXYZ());
  } else {
    _worldRotation = _localRotation;
    _worldScale = _localScale;

    t = SSE::Matrix4::translation(_localPosition);
  }

  const auto r = SSE::Matrix4::rotation(_worldRotation);
  const auto s = SSE::Matrix4::scale(_worldScale);

  _worldTransform = t * r * s;
}

Component& Entity::getComponent(size_t index) const { return *_components[index]; }

void Entity::lookAt(const Entity& other) { lookAt(other.position(), math::up); }

void Entity::lookAt(const SSE::Vector3& position, const SSE::Vector3& worldUp) {
  SSE::Vector3 forward;
  if (hasParent()) {
    const auto parentWorldInv = SSE::inverse(_parent->_worldTransform);
    forward = (parentWorldInv * position).getXYZ() - _localPosition;
  } else {
    forward = position - _localPosition;
  }

  if (SSE::lengthSqr(forward) == 0) {
    return;
  }

  // The DirectX LookAt function creates a view matrix; we want to create a transform
  // to rotate this object towards a target. So, we need to create the matrix manually.
  // (This also gives us the benefit of skipping the translation calculations; since they
  // are not used)

  forward = SSE::normalize(forward);

  SSE::Vector3 right = SSE::normalize(SSE::cross(forward, worldUp));
  SSE::Vector3 up = SSE::cross(right, forward);

  auto camToWorld = SSE::Matrix3(right, up, -forward);

  _localRotation = SSE::Quat(camToWorld);

  computeWorldTransform();
}

Component& Entity::addCloneOfComponent(const Component& srcComponent) {
  return _componentFactory.cloneComponentToEntity(srcComponent, *this);
}

void Entity::setActive(bool bActive) { _active = bActive; }

void Entity::setParent(Entity& newParent) {
  // TODO: Check that we don't create a cycle?
  if (hasParent()) {
    removeParent();
  }

  const auto thisPtr = _sceneGraph.getEntityPtrById(getId());
  _sceneGraph.removeFromRoot(thisPtr);

  auto newParentPtr = _sceneGraph.getEntityPtrById(newParent.getId());
  newParentPtr->_children.push_back(thisPtr);

  _parent = newParentPtr.get();
}

void Entity::removeParent() {
  if (_parent == nullptr) {
    return;
  }

  Entity_ptr_vec& children = _parent->_children;
  for (auto it = children.begin(); it != children.end(); ++it) {
    const auto child = (*it).get();
    if (child->getId() == getId()) {
      children.erase(it);
      break;
    }
  }

  const auto thisPtr = _sceneGraph.getEntityPtrById(getId());
  _sceneGraph.addToRoot(thisPtr);
  _parent = nullptr;
}

SSE::Vector3 Entity::worldPosition() const { return _worldTransform.getTranslation(); }

const SSE::Vector3& Entity::worldScale() const { return _worldScale; }

const SSE::Quat& Entity::worldRotation() const { return _worldRotation; }
void Entity::setTransform(const SSE::Matrix4& transform) {
  decompose_matrix(transform, _localPosition, _localRotation, _localScale);
}

std::shared_ptr<Entity> Entity::verifyEntityPtr() const {
  const auto thisPtr = _sceneGraph.getEntityPtrById(getId());
  if (!thisPtr) {
    OE_THROW(std::runtime_error("Attempting to access deleted Entity (id=" + std::to_string(getId()) + ")"));
  }

  return thisPtr;
}

Entity& EntityRef::get() const {
  const auto ptr = sceneGraph.getEntityPtrById(id);
  if (!ptr)
    OE_THROW(
        std::runtime_error("Attempting to access deleted Entity (id=" + std::to_string(id) + ")"));
  return *ptr.get();
}

#pragma once

#include <OeCore/Collision.h>
#include <OeCore/Component.h>

#include "vectormath/vectormath.hpp"

#include <map>
#include <vector>

namespace oe {

class Entity_repository;
class Scene;
namespace internal {
class Scene_graph_manager;
}

enum class Entity_state : uint8_t { Uninitialized, Initialized, Ready, Destroyed };

/**
 * A node in the scenegraph. This class is not designed to be extended via polymorphism;
 * functionality and behaviors should be added in the form of Components (for storing state)
 * and GameServices (for executing behavior).
 */
class Entity : public std::enable_shared_from_this<Entity> {
 public:
  using Id_type = unsigned int;
  using Entity_ptr_vec = std::vector<std::shared_ptr<Entity>>;
  using Entity_ptr_map = std::map<Id_type, std::shared_ptr<Entity>>;

  Entity(Scene& scene, std::string&& name, Id_type id);

  // Don't allow direct copy of Entity objects (we have a unique_ptr list of components).
  Entity(const Entity& that) = delete;

  /* If true, Update method will be called by parent during recursion. */
  bool isActive() const { return _active; }
  void setActive(bool bActive);

  Entity_state getState() const { return _state; }

  std::shared_ptr<Entity> parent() const {
    return hasParent() ? _parent->verifyEntityPtr() : nullptr;
  }

  void removeParent();
  bool hasParent() const { return _parent != nullptr; }
  void setParent(Entity& newParent);

  bool hasChildren() const { return !_children.empty(); }
  const Entity_ptr_vec& children() const { return _children; }

  /**
   * Computes the world transformation matrix for just this entity. (Non-recursive)
   */
  void computeWorldTransform();

  const Id_type& getId() const { return _id; }
  const std::string& getName() const { return _name; }

  size_t getComponentCount() const { return _components.size(); }
  Component& getComponent(size_t index) const;

  template <typename TComponent>
  std::vector<std::reference_wrapper<TComponent>> getComponentsOfType() const;

  /* Math Functions */
  void lookAt(const Entity& other);
  void lookAt(const SSE::Vector3& position, const SSE::Vector3& worldUp);

  /**
   * returns a nullptr if no component of given type was found.
   */
  template <typename TComponent> TComponent* getFirstComponentOfType() const;

  template <typename TComponent> TComponent& addComponent();

  Component& addCloneOfComponent(const Component& component);

  // Rotation from the forward vector, in local space
  const SSE::Quat& rotation() const { return _localRotation; }
  void setRotation(const SSE::Quat& vector) { _localRotation = vector; }

  // Translation from the origin, in local space
  const SSE::Vector3& position() const { return _localPosition; }
  void setPosition(const SSE::Vector3& vector) { _localPosition = vector; }

  // Scale, in local space
  const SSE::Vector3& scale() const { return _localScale; }
  void setScale(const SSE::Vector3& vector) { _localScale = vector; }
  void setScale(float scale) { _localScale = SSE::Vector3(scale, scale, scale); }

  void setTransform(const SSE::Matrix4& transform);

  bool calculateBoundSphereFromChildren() const { return _calculateBoundSphereFromChildren; }
  void setCalculateBoundSphereFromChildren(bool calculateBoundSphereFromChildren) {
    _calculateBoundSphereFromChildren = calculateBoundSphereFromChildren;
  }

  bool calculateWorldTransform() const { return _calculateWorldTransform; }
  void setCalculateWorldTransform(bool calculateWorldTransform) {
    _calculateWorldTransform = calculateWorldTransform;
  }

  const oe::BoundingSphere& boundSphere() const { return _boundSphere; }
  void setBoundSphere(const oe::BoundingSphere& boundSphere) { _boundSphere = boundSphere; }

  Scene& scene() const { return _scene; }

  /*
   * Returns the right handed world transform matrix (T*R*S).
   * If this Entity has no parent, this equals the local transform matrix.
   * Otherwise, this is localTransform * parent.worldTransform.
   */
  // TODO: This is still a LH Matrix...? :(
  const SSE::Matrix4& worldTransform() const { return _worldTransform; }
  void setWorldTransform(const SSE::Matrix4& worldTransform) { _worldTransform = worldTransform; }

  const SSE::Vector3& worldScale() const;
  SSE::Vector3 worldPosition() const;
  const SSE::Quat& worldRotation() const;

 private:
  std::shared_ptr<Entity> verifyEntityPtr() const;

  // TODO: Refactor into a public & private interface, so that friend isn't required.
  friend class Entity_repository;
  friend class internal::Scene_graph_manager;

  ////
  // Persisted State Variables
  ////
  SSE::Quat _localRotation;
  SSE::Vector3 _localPosition;
  SSE::Vector3 _localScale;

  // If true, the application will calculate the world transform from parent and TRS.
  // If false, the world transform must be updated manually
  bool _calculateWorldTransform;

  // If true, value of _boundSphere will be calculated to be the merged result of all children.
  // If false, the app must provide the value for _boundSphere.
  bool _calculateBoundSphereFromChildren;

  Id_type _id;
  const std::string _name;
  Entity_state _state;
  bool _active;

  Entity_ptr_vec _children;
  Entity* _parent;
  std::shared_ptr<Entity> _prefab;
  Scene& _scene;

  std::vector<std::unique_ptr<Component>> _components;
  ////
  // Runtime, generated variables
  ////
  // Generated by a call to ComputeWorldTransform
  SSE::Matrix4 _worldTransform;
  SSE::Quat _worldRotation;
  SSE::Vector3 _worldScale;

  BoundingSphere _boundSphere;

  /*
   * Redirect events to the Scene.
   */
  void onComponentAdded(Component& component);
};

template <typename TComponent>
std::vector<std::reference_wrapper<TComponent>> Entity::getComponentsOfType() const {
  std::vector<std::reference_wrapper<TComponent>> comps;
  for (auto iter = _components.begin(); iter != _components.end(); ++iter) {
    TComponent* comp = dynamic_cast<TComponent*>((*iter).get());
    if (comp != nullptr)
      comps.push_back(std::reference_wrapper<TComponent>(*comp));
  }
  return comps;
}

template <typename TComponent> TComponent* Entity::getFirstComponentOfType() const {
  for (auto iter = _components.begin(); iter != _components.end(); ++iter) {
    const auto comp = dynamic_cast<TComponent*>((*iter).get());
    if (comp != nullptr)
      return comp;
  }
  return nullptr;
}

template <typename TComponent> TComponent& Entity::addComponent() {
  TComponent* component = new TComponent(*this);
  _components.push_back(std::unique_ptr<Component>(component));

  this->onComponentAdded(*component);

  return *component;
}

struct EntityRef {
  Scene& scene;
  Entity::Id_type id;

  EntityRef(Entity& entity) : scene(entity.scene()), id(entity.getId()) {}

  EntityRef(Scene& scene, Entity::Id_type id) : scene(scene), id(id) {}

  Entity& get() const;

  Entity& operator*() const { return get(); }
};
} // namespace oe

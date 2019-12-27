// ReSharper disable CppClangTidyCppcoreguidelinesMacroUsage
#pragma once

namespace oe {
class Entity;
class Scene;

/**
 * Abstract base class for components, which are added to entities.
 */
class Component {
 public:
  using Component_type = unsigned int;

  explicit Component(Entity& entity) : _entity(entity) {}
  virtual ~Component() = default;
  Component(const Component&) = default;
  Component& operator=(const Component&) = delete;
  Component(Component&&) = default;
  Component& operator=(Component&&) = delete;

  /** Returns a new, unique component id */
  static Component_type createComponentTypeId() noexcept;

  [[nodiscard]] virtual Component_type getType() const = 0;
  [[nodiscard]] Entity& entity() const { return _entity; }

 private:
  static Component_type _maxComponentId;
  Entity& _entity;
};
} // namespace oe

#define DECLARE_COMPONENT_TYPE                                                                     \
 public:                                                                                           \
  Component_type getType() const override;                                                         \
  static Component_type type();                                                                    \
                                                                                                   \
 private:                                                                                          \
  static Component::Component_type _typeId;

#define DEFINE_COMPONENT_TYPE(classname)                                                           \
  Component::Component_type classname::_typeId = Component::createComponentTypeId();               \
  classname::Component_type classname::getType() const { return _typeId; }                         \
  classname::Component_type classname::type()                                                      \
  {                                                                                                \
    return _typeId;                                                                                \
  }

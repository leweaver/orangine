// ReSharper disable CppClangTidyCppcoreguidelinesMacroUsage
#pragma once

#include <functional>

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

  [[nodiscard]] virtual Component_type getType() const = 0;
  [[nodiscard]] Entity& getEntity() { return _entity; }
  [[nodiscard]] const Entity& getEntity() const { return _entity; }

  virtual std::unique_ptr<Component> clone(Entity& entity) const = 0;

 private:
  Entity& _entity;
};

class Component_factory {
 public:
  using Component_type = unsigned int;

  static void initStatics();
  static void destroyStatics();

  /** Registers a component type, returning the TypeId it should use. */
  static Component_type createComponentTypeId(std::unique_ptr<oe::Component>(*createInstance)(Entity&));

  static std::unique_ptr<oe::Component> createComponent(Component_type typeId, Entity& entity);

 private:
  static std::vector<std::unique_ptr<oe::Component>(*)(Entity&)> _factories;
  static Component_type _maxComponentId;
  static bool _staticsInitialized;
};
} // namespace oe

#define DECLARE_COMPONENT_TYPE                                         \
 public:                                                               \
  Component_type getType() const override;                             \
  static void initStatics();                                           \
  static void destroyStatics();                                        \
  static std::unique_ptr<Component> createInstance(oe::Entity& entity);\
  template<typename TClass> static std::unique_ptr<TClass> createInstanceTyped(oe::Entity& entity);\
  static Component_type type();                                        \
  std::unique_ptr<Component> clone(oe::Entity& entity) const override; \
                                                                       \
 private:                                                              \
  static Component::Component_type _typeId;

#define DEFINE_COMPONENT_TYPE(classname)                                             \
  void classname::initStatics() {                                                    \
    _typeId = Component_factory::createComponentTypeId(&classname::createInstance);  \
  }                                                                                  \
  void classname::destroyStatics() { }                                               \
  classname::Component_type classname::_typeId = 0;                                  \
  classname::Component_type classname::getType() const { return _typeId; }           \
  classname::Component_type classname::type() { return _typeId; }                    \
  std::unique_ptr<Component> classname::createInstance(oe::Entity& entity) {         \
    return createInstanceTyped<classname>(entity);                                   \
  }                                                                                  \
  template<typename TClass>                                                          \
  std::unique_ptr<TClass> classname::createInstanceTyped(oe::Entity& entity) {       \
    return std::make_unique<classname>(entity);                                      \
  }                                                                                  \
  std::unique_ptr<Component> classname::clone(oe::Entity& entity) const {            \
    auto instance = createInstanceTyped<classname>(entity);                          \
    instance->_component_properties = _component_properties;                         \
    return instance;                                                                 \
  }

#define BEGIN_COMPONENT_PROPERTIES() struct ComponentProperties {

#define END_COMPONENT_PROPERTIES() \
  }                                \
  _component_properties;
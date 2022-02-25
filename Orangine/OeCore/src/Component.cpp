#include "OeCore/Component.h"

using namespace oe;

bool Component_factory::_staticsInitialized = false; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::vector<std::unique_ptr<oe::Component>(*)(Entity&)> Component_factory::_factories {}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
Component_factory::Component_type Component_factory::_maxComponentId = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void Component_factory::initStatics() {
  _maxComponentId = 0;
  _factories = {};
  _staticsInitialized = true;
}

void Component_factory::destroyStatics() {
  _factories = {};
  _staticsInitialized = false;
}

oe::Component::Component_type oe::Component_factory::createComponentTypeId(std::unique_ptr<oe::Component>(*createInstance)(Entity&)) {
  assert(_staticsInitialized);
  _factories.push_back(createInstance);
  return _maxComponentId++;
}

std::unique_ptr<oe::Component> oe::Component_factory::createComponent(Component_type typeId, Entity& entity) {
  return _factories.at(typeId)(entity);
}
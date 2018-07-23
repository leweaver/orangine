#include "pch.h"
#include "Component.h"

unsigned int oe::Component::_maxComponentId = 0;

oe::Component::Component_type oe::Component::createComponentTypeId() { return ++_maxComponentId; }

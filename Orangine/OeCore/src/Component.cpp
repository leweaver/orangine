#include "pch.h"
#include "OeCore/Component.h"

unsigned int oe::Component::_maxComponentId = 0;

oe::Component::Component_type oe::Component::createComponentTypeId() noexcept { return ++_maxComponentId; }

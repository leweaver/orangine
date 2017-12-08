#include "pch.h"
#include "Component.h"

unsigned int OE::Component::ms_maxComponentId = 0;

OE::Component::ComponentType OE::Component::CreateComponentTypeId() { return ++ms_maxComponentId; }

//
// Created by Lewis on 7/3/2019.
//

#include "pch.h"

#include <OeScripting/Script_component.h>

#include "Script_runtime_data.h"

using namespace oe;

DEFINE_COMPONENT_TYPE(Script_component)

void Script_component::Script_runtime_data_deleter::operator()(
    ::oe::internal::Script_runtime_data* _Ptr) const noexcept {
  delete _Ptr;
}

const std::string& Script_component::scriptName() const { return _component_properties.scriptName; }

void Script_component::setScriptName(const std::string& scriptName) {
  _component_properties.scriptName = scriptName;
}
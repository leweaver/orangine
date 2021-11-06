#include "pch.h"

#include "OeCore/Clear_gbuffer_material.h"

using namespace oe;
using namespace std::literals;

const std::string g_material_type = "Clear_gbuffer_material";

Clear_gbuffer_material::Clear_gbuffer_material()
    : Base_type(static_cast<uint8_t>(Material_type_index::Clear_g_buffer)) {}

const std::string& Clear_gbuffer_material::materialType() const { return g_material_type; }

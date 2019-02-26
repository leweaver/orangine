#include "pch.h"
#include "Clear_gbuffer_material.h"
#include "Render_pass_config.h"
#include "Material_repository.h"

using namespace oe;
using namespace std::literals;

const std::string g_material_type = "Clear_gbuffer_material";

Clear_gbuffer_material::Clear_gbuffer_material()
    : Base_type(static_cast<uint8_t>(Material_type_index::Clear_G_Buffer))
{    
}

const std::string& Clear_gbuffer_material::materialType() const
{
	return g_material_type;
}

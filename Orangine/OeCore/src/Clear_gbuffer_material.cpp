#include "OeCore/Clear_gbuffer_material.h"

using namespace oe;
using namespace std::literals;

// TODO: don't use global std::string
const std::string g_material_type = "Clear_gbuffer_material";

Clear_gbuffer_material::Clear_gbuffer_material()
    : Material(static_cast<uint8_t>(Material_type_index::Clear_g_buffer)) {}

const std::string& Clear_gbuffer_material::materialType() const { return g_material_type; }

void Clear_gbuffer_material::updatePerDrawConstantBuffer(
        gsl::span<uint8_t> cpuBuffer, const Shader_layout_constant_buffer& bufferDesc,
        const Material::Update_constant_buffer_inputs& inputs)
{
  OE_CHECK_MSG(false, "Clear_gbuffer_material does not have any constant buffers to update");
}

Shader_constant_layout Clear_gbuffer_material::getShaderConstantLayout() const { return Shader_constant_layout(); }

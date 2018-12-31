#include "pch.h"
#include "Skybox_material.h"

using namespace oe;

const std::string g_material_type = "Skybox_material";

const std::string& Skybox_material::materialType() const
{
	return g_material_type;
}


#include "pch.h"
#include "Material_repository.h"
#include "PBR_material.h"

using namespace oe;

std::unique_ptr<Material> Material_repository::instantiate(const std::string& materialName) const
{
	// TODO: Look up by name. For now, hard coded!!
	(void)materialName;

	return std::make_unique<PBR_material>();
}

#include "pch.h"
#include "MaterialRepository.h"
#include "PBRMaterial.h"

using namespace OE;

std::unique_ptr<Material> MaterialRepository::instantiate(const std::string& materialName) const
{
	// TODO: Look up by name. For now, hard coded!!
	(void)materialName;

	return std::make_unique<PBRMaterial>();
}

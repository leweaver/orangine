#include "pch.h"
#include "MaterialRepository.h"

std::unique_ptr<OE::Material> OE::MaterialRepository::Instantiate(const std::string& materialName) const
{
	// TODO: Look up by name. For now, hard coded!!
	(void)materialName;

	return std::make_unique<Material>();
}

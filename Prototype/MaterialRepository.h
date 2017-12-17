#pragma once
#include "Material.h"
#include <map>

namespace OE {
	class MaterialRepository
	{
	public:
		/**
		 * Will return a new Error Material if the given material doesn't exist.
		 */
		std::unique_ptr<Material> Instantiate(const std::string &materialName) const;
	};
}

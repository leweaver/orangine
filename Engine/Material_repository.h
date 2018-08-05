#pragma once

#include "Material.h"

namespace oe {

	class IMaterial_repository {
	public:
		virtual std::unique_ptr<Material> instantiate(const std::string& materialName) const = 0;
	};

	class Material_repository : public IMaterial_repository {
	public:
		/**
		 * Will return a new Error Material if the given material doesn't exist.
		 */
		std::unique_ptr<Material> instantiate(const std::string& materialName) const override;
	};
}

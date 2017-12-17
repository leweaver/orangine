#pragma once

#include "EntityGraphLoader.h"

namespace OE {
	class Entity;

	class glTFMeshLoader : public EntityGraphLoader
	{
	public:
		void GetSupportedFileExtensions(std::vector<std::string> &extensions) const override;
		std::vector<std::shared_ptr<Entity>> LoadFile(const std::string &path, EntityRepository &entityFactor, MaterialRepository &materialRepositoryy) const override;
	};

}

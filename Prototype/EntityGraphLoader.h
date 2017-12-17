#pragma once

#include <memory>
#include <vector>

namespace DX
{
	class DeviceResources;
}

namespace OE
{
	class Entity;
	class EntityRepository;
	class MaterialRepository;

	class EntityGraphLoader
	{
	public:
		virtual ~EntityGraphLoader() {}

		virtual void GetSupportedFileExtensions(std::vector<std::string> &extensions) const = 0;

		/**
		 * Returns a vector of root entities, that may have children. These entities will have been initialized such that they point to a scene,  
		 * but they will not have been added to that scene yet.
		 */
		virtual std::vector<std::shared_ptr<Entity>> LoadFile(const std::string &path, EntityRepository &entityFactory, MaterialRepository &materialRepository) const = 0;
	};
}
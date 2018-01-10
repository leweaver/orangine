#pragma once

#include "EntityGraphLoader.h"

struct IWICImagingFactory;

namespace OE {
	class Entity;

	class glTFMeshLoader : public EntityGraphLoader
	{

		Microsoft::WRL::ComPtr<IWICImagingFactory> m_imagingFactory;

	public:
		glTFMeshLoader();

		void GetSupportedFileExtensions(std::vector<std::string> &extensions) const override;
		std::vector<std::shared_ptr<Entity>> LoadFile(const std::string &path, EntityRepository &entityFactor, MaterialRepository &materialRepositoryy) const override;
	};

}

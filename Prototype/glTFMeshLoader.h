#pragma once

#include "MeshLoader.h"

namespace OE {
	class glTFMeshLoader : public MeshLoader
	{
	public:
		void GetSupportedFileExtensions(std::vector<std::string> &extensions) const override;
		std::unique_ptr<RendererData> LoadFile(const std::string &path, const DX::DeviceResources &deviceResources) const override;
	};

}

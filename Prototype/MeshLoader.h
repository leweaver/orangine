#pragma once

#include <memory>
#include "RendererData.h"

namespace DX
{
	class DeviceResources;
}

namespace OE
{
	class MeshLoader
	{
	public:
		virtual ~MeshLoader() {}

		virtual void GetSupportedFileExtensions(std::vector<std::string> &extensions) const = 0;
		virtual std::unique_ptr<RendererData> LoadFile(const std::string &path, const DX::DeviceResources &deviceResources) const = 0;
	};
}
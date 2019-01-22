#pragma once

#include <memory>

namespace oe {
	class Mesh_data;
	class Material;
	struct Renderer_data;

	struct Renderable
	{
		std::shared_ptr<Mesh_data> meshData;
		std::shared_ptr<Material> material;
		std::unique_ptr<Renderer_data> rendererData;
	};
}
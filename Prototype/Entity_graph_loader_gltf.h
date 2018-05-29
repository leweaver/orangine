#pragma once

#include "Entity_graph_loader.h"

struct IWICImagingFactory;

namespace oe {
	class Entity;

	class Entity_graph_loader_gltf : public Entity_graph_loader {
	public:
		Entity_graph_loader_gltf();

		void getSupportedFileExtensions(std::vector<std::string>& extensions) const override;
		std::vector<std::shared_ptr<Entity>> loadFile(std::string_view filename,
			Entity_repository& entityRepository,
			Material_repository& materialRepository,
			Primitive_mesh_data_factory& meshDataFactory) const override;

	private:
		Microsoft::WRL::ComPtr<IWICImagingFactory> _imagingFactory;
	};

}

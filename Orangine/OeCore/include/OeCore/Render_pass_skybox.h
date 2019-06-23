#pragma once
#include "Render_pass.h"
#include "Asset_manager.h"
#include "Mesh_data.h"
#include <memory>
#include "Skybox_material.h"

namespace oe {
	struct Renderable;
	class Scene;

	class Render_pass_skybox : public Render_pass {
	public:
		explicit Render_pass_skybox(Scene& scene);

		void render(const Camera_data& cameraData) override;
		
		void createDeviceDependentResources() override;
		void destroyDeviceDependentResources() override;

	private:
		Scene& _scene;

		std::shared_ptr<Texture> _cubemap = nullptr;
		std::shared_ptr<Mesh_data> _meshData = nullptr;
		std::shared_ptr<Skybox_material> _material = nullptr;

		std::unique_ptr<Renderable> _renderable = nullptr;
	};
}

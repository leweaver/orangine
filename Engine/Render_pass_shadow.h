#pragma once
#include "Render_pass.h"
#include "Render_light_data.h"

#include <memory>
#include <vector>

namespace oe {

	class IShadowmap_manager;
	class Entity_filter;
	class Scene;

	class Render_pass_shadow : public Render_pass {
	public:
		Render_pass_shadow(
			Scene& scene,
			size_t max_render_maxRenderTargetViewstarget_views
		);

		virtual void render(const Camera_data& cameraData);

	private:
		Scene& _scene;

		std::shared_ptr<Entity_filter> _renderableEntities;
		std::shared_ptr<Entity_filter> _lightEntities;

		std::vector<ID3D11RenderTargetView*> _renderTargetViews;
	};
}

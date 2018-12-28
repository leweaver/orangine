﻿#pragma once
#include "Render_pass.h"
#include "Render_light_data.h"

#include <memory>
#include <vector>

namespace oe {

	class Entity_filter;
	class Scene;

	class Render_pass_shadow : public Render_pass {
	public:
		Render_pass_shadow(
			Scene& scene,
			size_t maxRenderTargetViews
		);

		void render(const Camera_data& cameraData) override;

	private:
		Scene& _scene;

		std::shared_ptr<Entity_filter> _renderableEntities;
		std::shared_ptr<Entity_filter> _lightEntities;

		std::vector<ID3D11RenderTargetView*> _renderTargetViews;
	};
}

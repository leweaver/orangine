#pragma once

#include "Collision.h"
#include "Render_target_texture.h"

namespace oe {
	class Renderer_shadow_data {
	public:
		const BoundingFrustumRH& frustum() const { return _frustum; }
		void setFrustum(const BoundingFrustumRH& frustum) { _frustum = frustum; }

		const std::shared_ptr<Render_target_texture>& texture() const { return _texture; }
		void setTexture(std::shared_ptr<Render_target_texture> texture) { _texture = texture; }

	private:
		BoundingFrustumRH _frustum;
		std::shared_ptr<Render_target_texture> _texture;
	};
}
#pragma once

#include "Render_pass.h"
#include <functional>

namespace oe {
	class Render_pass_generic : public Render_pass {
	public:
		Render_pass_generic(std::function<void(const Camera_data&)> render) 
			: _render(render) 
		{}

		virtual void render(const Camera_data& cameraData)
		{
			_render(cameraData);
		}

	private:
		std::function<void(const Camera_data&)> _render;
	};
}

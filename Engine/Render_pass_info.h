#pragma once

#include "Renderer_enum.h"

namespace oe
{
	template<Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode>
	struct Render_pass_info {
		const Render_pass_blend_mode blendMode = TBlend_mode;
		const Render_pass_depth_mode depthMode = TDepth_mode;

		std::function<void()> render = [](){};

		constexpr static bool supportsBlendedAlpha()
		{
			return TBlend_mode == Render_pass_blend_mode::Blended_alpha;
		}

		constexpr static bool depthEnabled()
		{
			return TDepth_mode == Render_pass_depth_mode::Enabled;
		}
	};
}

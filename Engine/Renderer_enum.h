#pragma once

namespace oe {

	enum class Material_alpha_mode
	{
		Opaque,
		Mask,
		Blend
	};

	enum class Render_pass_blend_mode {
		Opaque,
		Blended_alpha,
		Additive
	};

	enum class Render_pass_depth_mode {
		ReadWrite,
		ReadOnly,
		Disabled
	};
}
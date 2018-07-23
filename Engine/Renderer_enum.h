#pragma once

namespace oe {

	enum class Material_alpha_mode
	{
		Opaque,
		Mask,
		Blend
	};

	enum class Render_pass_output_format {
		Shaded_StandardLight,
		Shaded_DeferredLight,
		Shaded_Unlit
	};
}
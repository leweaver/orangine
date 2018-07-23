#pragma once

#include "Renderer_enum.h"

namespace oe
{
	template<Render_pass_output_format TFormat>
	struct Render_pass_info {

		const Render_pass_output_format outputFormat = TFormat;

		constexpr static bool supportsBlendedAlpha()
		{
			return false;
		}
	};

	template <>
	constexpr bool Render_pass_info<Render_pass_output_format::Shaded_StandardLight>::supportsBlendedAlpha()
	{
		return true;
	}
}

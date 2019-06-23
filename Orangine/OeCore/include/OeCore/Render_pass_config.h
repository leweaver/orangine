#pragma once

#include "Renderer_enum.h"

namespace oe
{
	template<
		Render_pass_blend_mode TBlend_mode, 
		Render_pass_depth_mode TDepth_mode,
		Render_pass_stencil_mode TStencil_mode = Render_pass_stencil_mode::Disabled,
		uint32_t TStencil_read_mask = D3D11_DEFAULT_STENCIL_READ_MASK,
		uint32_t TStencil_write_mask = D3D11_DEFAULT_STENCIL_WRITE_MASK
	>
	struct Render_pass_config {
		constexpr static Render_pass_blend_mode blendMode()
		{
			return TBlend_mode;
		}

		constexpr static bool supportsBlendedAlpha()
		{
			return TBlend_mode == Render_pass_blend_mode::Blended_Alpha;
		}

		constexpr static Render_pass_depth_mode depthMode()
		{
			return TDepth_mode;
		}

		constexpr static Render_pass_stencil_mode stencilMode()
		{
			return TStencil_mode;
		}

		constexpr static uint32_t stencilReadMask()
		{
			return TStencil_read_mask;
		}

		constexpr static uint32_t stencilWriteMask()
		{
			return TStencil_write_mask;
		}
	};
}

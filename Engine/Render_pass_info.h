#pragma once

#include "Renderer_enum.h"
#include "Render_target_texture.h"

namespace oe
{
	template<Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode>
	struct Render_pass_info {
		std::function<void()> render = [](){};
		
		constexpr static Render_pass_blend_mode blendMode()
		{
			return TBlend_mode;
		}

		constexpr static bool supportsBlendedAlpha()
		{
			return TBlend_mode == Render_pass_blend_mode::Blended_alpha;
		}

		constexpr static Render_pass_depth_mode depthMode()
		{
			return TDepth_mode;
		}

		void setRenderTargets(std::vector<std::shared_ptr<Render_target_view_texture>>&& renderTargets);
		void clearRenderTargets();
		std::tuple<ID3D11RenderTargetView* const*, size_t> renderTargetViewArray() const;

	private:
		std::vector<std::shared_ptr<Render_target_view_texture>> _renderTargets = {};
		std::vector<ID3D11RenderTargetView*> _renderTargetViews = {};
	};

	template<Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode>
	void Render_pass_info<TBlend_mode, TDepth_mode>::setRenderTargets(
		std::vector<std::shared_ptr<Render_target_view_texture>>&& renderTargets)
	{
		for (auto i = 0; i < _renderTargetViews.size(); ++i) {
			if (_renderTargetViews[i]) {
				_renderTargetViews[i]->Release();
				_renderTargetViews[i] = nullptr;
			}
		}

		_renderTargets = move(renderTargets);
		_renderTargetViews.resize(_renderTargets.size(), nullptr);

		for (auto i = 0; i < _renderTargetViews.size(); ++i) {
			if (_renderTargets[i]) {
				_renderTargetViews[i] = _renderTargets[i]->renderTargetView();
				_renderTargetViews[i]->AddRef();
			}
		}
	}

	template<Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode>
	void Render_pass_info<TBlend_mode, TDepth_mode>::clearRenderTargets()
	{
		std::vector<std::shared_ptr<Render_target_view_texture>> renderTargets;
		setRenderTargets(move(renderTargets));
	}

	template<Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode>
	std::tuple<ID3D11RenderTargetView* const*, size_t> Render_pass_info<TBlend_mode, TDepth_mode>::renderTargetViewArray() const
	{
		return {
			_renderTargetViews.data(),
			_renderTargetViews.size()
		};
	}
}

#include "pch.h"
#include "OeCore/Render_pass.h"
#include "OeCore/Render_target_texture.h"

using namespace oe;

// Static initialization order means we can't use Matrix::Identity here.
const Render_pass::Camera_data Render_pass::Camera_data::IDENTITY = {
	SSE::Matrix4::identity(),
	SSE::Matrix4::identity(),
};

void Render_pass::setRenderTargets(
	std::vector<std::shared_ptr<Render_target_view_texture>>&& renderTargets)
{
	for (size_t i = 0; i < _renderTargetViews.size(); ++i) {
		if (_renderTargetViews[i]) {
			_renderTargetViews[i]->Release();
			_renderTargetViews[i] = nullptr;
		}
	}

	_renderTargets = move(renderTargets);
	_renderTargetViews.resize(_renderTargets.size(), nullptr);

	for (size_t i = 0; i < _renderTargetViews.size(); ++i) {
		if (_renderTargets[i]) {
			_renderTargetViews[i] = _renderTargets[i]->renderTargetView();
			_renderTargetViews[i]->AddRef();
		}
	}
}

void Render_pass::clearRenderTargets()
{
	std::vector<std::shared_ptr<Render_target_view_texture>> renderTargets;
	setRenderTargets(move(renderTargets));
}

std::tuple<ID3D11RenderTargetView* const*, size_t>
Render_pass::renderTargetViewArray() const
{
	return {
		_renderTargetViews.data(),
		_renderTargetViews.size()
	};
}

void Render_pass::setBlendState(ID3D11BlendState* blendState)
{
	_blendState = blendState;
}

void Render_pass::setDepthStencilState(ID3D11DepthStencilState* depthStencilState)
{
	_depthStencilState = depthStencilState;
}

#include "pch.h"

#include "OeCore/Render_pass.h"
#include "OeCore/Texture.h"

using namespace oe;

// Static initialization order means we can't use Matrix::Identity here.
const Camera_data Camera_data::IDENTITY = {
    SSE::Matrix4::identity(),
    SSE::Matrix4::identity(),
};

void Render_pass::setRenderTargets(const std::vector<std::shared_ptr<Texture>>& renderTargets) {
  _renderTargets = renderTargets;
  _renderTargetsChanged = true;
}

void Render_pass::clearRenderTargets() {
  setRenderTargets({});
}

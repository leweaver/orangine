#pragma once

#include "Render_pass.h"

#include <functional>

namespace oe {
class Render_pass_generic : public Render_pass {
 public:
  Render_pass_generic(std::function<void(const Camera_data&, const Render_pass& pass)> render)
      : _render(render) {}

  void render(const Camera_data& cameraData) override { _render(cameraData, *this); }

 private:
  std::function<void(const Camera_data&, const Render_pass&)> _render;
};
} // namespace oe

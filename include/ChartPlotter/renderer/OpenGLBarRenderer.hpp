#pragma once

#include "ChartPlotter/renderer/IOpenGLRenderer.hpp"

namespace ChartPlotter {

class OpenGLBarRenderer : public IOpenGLRenderer {
public:
  OpenGLBarRenderer() = default;

  void initialize(QOpenGLExtraFunctions *f) override;
  void release(QOpenGLExtraFunctions *f) override;
  void render(const ChartRenderContext &context) override;
  void setData(std::unique_ptr<RenderData> data) override;
};

} // namespace ChartPlotter

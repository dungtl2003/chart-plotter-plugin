#include "ChartPlotter/renderer/OpenGLBarRenderer.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

namespace ChartPlotter {

void OpenGLBarRenderer::initialize(QOpenGLExtraFunctions *f) {
  CP_DEBUG("initialize");
}
void OpenGLBarRenderer::release(QOpenGLExtraFunctions *f) {
  CP_DEBUG("release");
}
void OpenGLBarRenderer::render(const ChartRenderContext &context) {
  CP_DEBUG("render");
}

void OpenGLBarRenderer::setData(std::unique_ptr<RenderData> data) {};

} // namespace ChartPlotter

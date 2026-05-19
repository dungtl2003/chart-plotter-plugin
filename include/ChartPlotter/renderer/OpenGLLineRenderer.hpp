#include "ChartPlotter/renderer/IChartRenderer.hpp"

class OpenGLLineRenderer : public IChartRenderer {
public:
  OpenGLLineRenderer() = default;

  void render() override;
};

#include "ChartPlotter/renderer/IChartRenderer.hpp"

class OpenGLBarRenderer : public IChartRenderer {
public:
  OpenGLBarRenderer() = default;

  void render() override;
};

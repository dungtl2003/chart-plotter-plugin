#include "ChartPlotter/factory/PieSeriesComponentFactory.hpp"
#include "ChartPlotter/renderer/OpenGLPieRenderer.hpp"
#include "ChartPlotter/strategy/PieSeriesStrategy.hpp"

namespace ChartPlotter {

std::unique_ptr<ISeriesStrategy>
PieSeriesComponentFactory::getStrategy() const {
  return std::make_unique<PieSeriesStrategy>();
}

std::unique_ptr<IOpenGLRenderer>
PieSeriesComponentFactory::getRenderer() const {
  return std::make_unique<OpenGLPieRenderer>();
}

} // namespace ChartPlotter

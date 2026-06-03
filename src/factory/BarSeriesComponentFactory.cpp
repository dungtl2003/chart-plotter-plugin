#include "ChartPlotter/factory/BarSeriesComponentFactory.hpp"
#include "ChartPlotter/renderer/OpenGLBarRenderer.hpp"
#include "ChartPlotter/strategy/BarSeriesStrategy.hpp"

namespace ChartPlotter {

std::unique_ptr<ISeriesStrategy>
BarSeriesComponentFactory::getStrategy() const {
  return std::make_unique<BarSeriesStrategy>();
}

std::unique_ptr<IOpenGLRenderer>
BarSeriesComponentFactory::getRenderer() const {
  return std::make_unique<OpenGLBarRenderer>();
}

} // namespace ChartPlotter

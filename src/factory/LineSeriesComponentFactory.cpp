#include "ChartPlotter/factory/LineSeriesComponentFactory.hpp"
#include "ChartPlotter/renderer/OpenGLLineRenderer.hpp"
#include "ChartPlotter/strategy/LineSeriesStrategy.hpp"

namespace ChartPlotter {

std::unique_ptr<ISeriesStrategy>
LineSeriesComponentFactory::getStrategy() const {
  return std::make_unique<LineSeriesStrategy>();
}

std::unique_ptr<IOpenGLRenderer>
LineSeriesComponentFactory::getRenderer() const {
  return std::make_unique<OpenGLLineRenderer>();
}

} // namespace ChartPlotter

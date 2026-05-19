#include "ChartPlotter/factory/LineChartComponentFactory.hpp"
#include "ChartPlotter/renderer/OpenGLLineRenderer.hpp"
#include "ChartPlotter/strategy/LineChartStrategy.hpp"

std::unique_ptr<IChartStrategy> LineChartComponentFactory::getStrategy() const {
  return std::make_unique<LineChartStrategy>();
}

std::unique_ptr<IChartRenderer> LineChartComponentFactory::getRenderer() const {
  return std::make_unique<OpenGLLineRenderer>();
}

#include "ChartPlotter/factory/BarChartComponentFactory.hpp"
#include "ChartPlotter/renderer/OpenGLBarRenderer.hpp"
#include "ChartPlotter/strategy/BarChartStrategy.hpp"

std::unique_ptr<IChartStrategy> BarChartComponentFactory::getStrategy() const {
  return std::make_unique<BarChartStrategy>();
}

std::unique_ptr<IChartRenderer> BarChartComponentFactory::getRenderer() const {
  return std::make_unique<OpenGLBarRenderer>();
}

#include "ChartPlotter/factory/ChartComponentFactoryProvider.hpp"
#include "ChartPlotter/factory/BarChartComponentFactory.hpp"
#include "ChartPlotter/factory/LineChartComponentFactory.hpp"

std::unique_ptr<IChartComponentFactory>
ChartComponentFactoryProvider::getFactory(ChartType type) {
  switch (type) {
  case ChartType::Line:
    return std::make_unique<LineChartComponentFactory>();
  case ChartType::Bar:
    return std::make_unique<BarChartComponentFactory>();
  default:
    return nullptr;
  }
}

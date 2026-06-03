#include "ChartPlotter/factory/SeriesComponentFactoryProvider.hpp"
#include "ChartPlotter/factory/BarSeriesComponentFactory.hpp"
#include "ChartPlotter/factory/LineSeriesComponentFactory.hpp"

namespace ChartPlotter {

std::unique_ptr<ISeriesComponentFactory>
SeriesComponentFactoryProvider::getFactory(ChartEnums::SeriesType type) {
  switch (type) {
  case ChartEnums::SeriesType::Line:
    return std::make_unique<LineSeriesComponentFactory>();
  case ChartEnums::SeriesType::Bar:
    return std::make_unique<BarSeriesComponentFactory>();
  default:
    return nullptr;
  }
}

} // namespace ChartPlotter

#include "ChartPlotter/factory/SeriesComponentFactoryProvider.hpp"
#include "ChartPlotter/factory/BarSeriesComponentFactory.hpp"
#include "ChartPlotter/factory/LineSeriesComponentFactory.hpp"
#include "ChartPlotter/factory/PieSeriesComponentFactory.hpp"

namespace ChartPlotter {

std::unique_ptr<ISeriesComponentFactory>
SeriesComponentFactoryProvider::getFactory(ChartEnums::SeriesType type) {
  switch (type) {
  case ChartEnums::SeriesType::Line:
    return std::make_unique<LineSeriesComponentFactory>();
  case ChartEnums::SeriesType::Bar:
    return std::make_unique<BarSeriesComponentFactory>();
  case ChartEnums::SeriesType::Pie:
    return std::make_unique<PieSeriesComponentFactory>();
  default:
    return nullptr;
  }
}

} // namespace ChartPlotter

#include "ChartPlotter/series/BarSeries.hpp"

namespace ChartPlotter {

BarSeries::BarSeries(QObject *parent) : XYSeries(parent) {}

ChartEnums::SeriesType BarSeries::type() { return ChartEnums::SeriesType::Bar; }

} // namespace ChartPlotter

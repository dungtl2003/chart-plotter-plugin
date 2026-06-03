#include "ChartPlotter/strategy/BarSeriesStrategy.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

namespace ChartPlotter {

std::unique_ptr<RenderData>
BarSeriesStrategy::build(const AbstractSeries &series,
                         const ResolvedSeriesData &resolved,
                         const DataSnapshot &snapshot) {
  CP_DEBUG("Building Bar Chart");
  return std::make_unique<XYSeriesRenderData>();
}

} // namespace ChartPlotter

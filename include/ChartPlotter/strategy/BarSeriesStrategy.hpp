#pragma once

#include "ChartPlotter/strategy/ISeriesStrategy.hpp"

namespace ChartPlotter {

class BarSeriesStrategy : public ISeriesStrategy {
public:
  std::unique_ptr<RenderData> build(const AbstractSeries &series,
                                    const ResolvedSeriesData &resolved,
                                    const DataSnapshot &snapshot,
                                    const SeriesBuildContext &context) override;
};

} // namespace ChartPlotter

#pragma once

#include "ChartPlotter/data/RenderData.hpp"
#include "ChartPlotter/series/AbstractSeries.hpp"
#include "ChartPlotter/series/ResolvedSeriesData.hpp"
#include "DataBuffer.hpp"

namespace ChartPlotter {

class ISeriesStrategy {
public:
  ISeriesStrategy() = default;

  explicit ISeriesStrategy(const ISeriesStrategy &) = delete;
  explicit ISeriesStrategy(ISeriesStrategy &&) = default;
  ISeriesStrategy &operator=(const ISeriesStrategy &) = delete;
  ISeriesStrategy &operator=(ISeriesStrategy &&) = delete;
  virtual ~ISeriesStrategy() = default;

  virtual std::unique_ptr<RenderData> build(const AbstractSeries &series,
                                            const ResolvedSeriesData &resolved,
                                            const DataSnapshot &snapshot) = 0;
};

} // namespace ChartPlotter

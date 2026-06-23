#pragma once

#include "ChartPlotter/axis/CategoryAxis.hpp"
#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/data/RenderData.hpp"
#include "ChartPlotter/series/AbstractSeries.hpp"
#include "ChartPlotter/series/ResolvedSeriesData.hpp"
#include "downsample/DataDownsampler.hpp"

namespace ChartPlotter {

struct SeriesBuildContext {
  // Non-null only when the corresponding axis is categorical (String).
  const CategoryAxis *xCategories = nullptr;
  const CategoryAxis *yCategories = nullptr; // reserved for categorical y
  float globalLineWidth = 2.0;
  float globalAntialiasing = 1.0;
  DataRange viewportXRange;
  std::unique_ptr<DataDownsampler> dataDownsampler;
  qsizetype preferredTotalPoints = 200;
};

class ISeriesStrategy {
public:
  ISeriesStrategy() = default;

  explicit ISeriesStrategy(const ISeriesStrategy &) = delete;
  explicit ISeriesStrategy(ISeriesStrategy &&) = default;
  ISeriesStrategy &operator=(const ISeriesStrategy &) = delete;
  ISeriesStrategy &operator=(ISeriesStrategy &&) = delete;
  virtual ~ISeriesStrategy() = default;

  virtual std::unique_ptr<RenderData>
  build(const AbstractSeries &series, const ResolvedSeriesData &resolved,
        const DataSnapshot &snapshot, const SeriesBuildContext &context) = 0;
};

} // namespace ChartPlotter

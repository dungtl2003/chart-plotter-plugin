#pragma once

#include "ChartPlotter/axis/CategoryAxis.hpp"
#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/data/RenderData.hpp"
#include "ChartPlotter/downsample/LodPyramid.hpp"
#include "ChartPlotter/series/AbstractSeries.hpp"
#include "ChartPlotter/series/ResolvedSeriesData.hpp"

#include <deque>

namespace ChartPlotter {

struct PointCacheKey {
  int sourceId;
  qint64 xColIndex;
  qint64 yColIndex;

  bool operator==(const PointCacheKey &other) const {
    return sourceId == other.sourceId && xColIndex == other.xColIndex &&
           yColIndex == other.yColIndex;
  }
};

inline size_t qHash(const PointCacheKey &key, size_t seed = 0) {
  return qHashMulti(seed, key.sourceId, key.xColIndex, key.yColIndex);
}

struct PointCacheValue {
  quint64 epochId = 0;
  quint64 processedRowCount = 0;
  // deque (not QVector) so sliding-window front eviction frees whole front nodes
  // in O(evicted) instead of memmoving the entire tail down on every batch.
  std::deque<QPointF> points;
  DataRange resolvedXRange;
  DataRange resolvedYRange;

  // LOD pyramid over `points`, kept in sync incrementally so zoom/pan is
  // O(visible buckets) instead of O(visible points). Serves both the min/max and
  // LTTB queries.
  LodPyramid lodPyramid;
};

struct SeriesBuildContext {
  // Non-null only when the corresponding axis is categorical (String).
  const CategoryAxis *xCategories = nullptr;
  const CategoryAxis *yCategories = nullptr; // reserved for categorical y
  float globalLineWidth = 2.0;
  float globalAntialiasing = 1.0;
  ChartEnums::DownsampleMode downsampleMode = ChartEnums::DownsampleMode::Lttb;
  DataRange viewportXRange;
  qsizetype preferredTotalPoints = 200;
  bool shouldDownsample = true;

  QHash<PointCacheKey, PointCacheValue> *globalPointCache;
};

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
                                            const DataSnapshot &snapshot,
                                            SeriesBuildContext &context) = 0;
};

} // namespace ChartPlotter

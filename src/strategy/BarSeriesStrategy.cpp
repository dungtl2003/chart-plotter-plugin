#include "ChartPlotter/strategy/BarSeriesStrategy.hpp"

#include "ChartPlotter/constants/ChartConstants.hpp"
#include "ChartPlotter/data/BarRenderData.hpp"
#include "ChartPlotter/series/BarSeries.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ChartPlotter {

std::unique_ptr<RenderData> BarSeriesStrategy::build(
    const AbstractSeries &series, const ResolvedSeriesData &resolved,
    const DataSnapshot &snapshot, SeriesBuildContext &context) {
  auto data = std::make_unique<BarRenderData>();

  if (!resolved.valid) {
    CP_WARN("BarSeriesStrategy::build: resolved series is invalid: {}",
            resolved.errorMessage.toStdString());
    return data;
  }

  const int xIndex = resolved.xColumnIndex;
  const int yIndex = resolved.yColumnIndex;

  if (xIndex < 0 || xIndex >= snapshot.columnCount || yIndex < 0 ||
      yIndex >= snapshot.columnCount) {
    CP_WARN("BarSeriesStrategy::build: invalid column index");
    return data;
  }

  if (resolved.yColumnType != ChartEnums::DataType::Number) {
    CP_WARN("BarSeriesStrategy::build: y column must be Number");
    return data;
  }

  const auto *barSeries = qobject_cast<const BarSeries *>(&series);
  if (!barSeries) {
    CP_WARN("BarSeriesStrategy::build: series is not BarSeries");
    return data;
  }

  data->color = barSeries->color();
  data->baseline = 0.0;

  const ChartEnums::DataType xType = resolved.xColumnType;
  const bool xIsCategory = xType == ChartEnums::DataType::String;

  const bool hasViewport = context.viewportXRange.valid;
  const double viewMin = context.viewportXRange.min;
  const double viewMax = context.viewportXRange.max;

  data->points.reserve(snapshot.rowCount);

  for (qint64 row = 0; row < snapshot.rowCount; ++row) {
    double x = snapshot.valueAt(xIndex, row);
    const double y = snapshot.valueAt(yIndex, row);

    if (std::isnan(x) || std::isnan(y)) {
      continue;
    }

    if (xIsCategory) {
      const int localId = static_cast<int>(x);
      if (localId < 0 || localId >= resolved.localToGlobalXMap.size()) {
        continue;
      }
      x = resolved.localToGlobalXMap[localId];
    }

    // Keep only bars whose center is inside the visible X range (small pad so
    // bars at the edge are not clipped away too eagerly).
    if (hasViewport && (x < viewMin - 1.0 || x > viewMax + 1.0)) {
      continue;
    }

    data->points.append(QPointF(x, y));
  }

  // The baseline must be part of the Y range so bars are anchored at 0.
  DataRange::includeValue(data->yRange, data->baseline);

  for (const QPointF &p : data->points) {
    DataRange::includeValue(data->xRange, p.x());
    DataRange::includeValue(data->yRange, p.y());
  }

  if (data->points.isEmpty()) {
    // Still return valid empty data so axes can render.
    DataRange::includeValue(data->xRange, hasViewport ? viewMin : 0.0);
    DataRange::includeValue(data->xRange, hasViewport ? viewMax : 1.0);
  }

  // Bar slot width in data units. Category ids are consecutive integers, so
  // each band is exactly 1 wide. For a numeric x axis, use the smallest gap
  // between neighbouring values so bars stay packed without overlapping. The
  // renderer turns this into pixels against the live axis range, so width
  // follows zoom.
  if (xIsCategory) {
    data->bandWidth = 1.0;
  } else if (data->points.size() > 1) {
    QVector<double> xs;
    xs.reserve(data->points.size());
    for (const QPointF &p : data->points) {
      xs.append(p.x());
    }
    std::sort(xs.begin(), xs.end());

    double minGap = std::numeric_limits<double>::max();
    for (qsizetype i = 1; i < xs.size(); ++i) {
      const double gap = xs[i] - xs[i - 1];
      if (gap > ChartConstants::EPSILON) {
        minGap = std::min(minGap, gap);
      }
    }
    if (minGap != std::numeric_limits<double>::max()) {
      data->bandWidth = minGap;
    }
  }

  data->valid = true;
  return data;
}

} // namespace ChartPlotter

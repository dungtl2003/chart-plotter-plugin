#include "ChartPlotter/strategy/LineSeriesStrategy.hpp"

#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/series/LineSeries.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QPointF>
#include <QVector>

#include <algorithm>
#include <memory>

namespace ChartPlotter {

std::unique_ptr<RenderData> LineSeriesStrategy::build(
    const AbstractSeries &series, const ResolvedSeriesData &resolved,
    const DataSnapshot &snapshot, SeriesBuildContext &context) {
  auto data = std::make_unique<LineRenderData>();

  auto result = validateTyAndColIndex(resolved, snapshot);
  if (!result) {
    CP_WARN(result.error().toStdString());
    return data;
  }

  result = loadSeriesConfig(data.get(), series, context);
  if (!result) {
    CP_WARN(result.error().toStdString());
    return data;
  }

  PointCacheKey key{resolved.sourceId, resolved.xColumnIndex,
                    resolved.yColumnIndex};
  PointCacheValue &cache = (*context.globalPointCache)[key];

  if (cache.epochId != snapshot.epochId) {
    cache.epochId = snapshot.epochId;
    cache.processedRowCount = 0;
    cache.points.clear();
  }

  if (cache.processedRowCount < snapshot.rowCount) {
    // CP_DEBUG("Processing new delta rows...");
    QVector<QPointF> newPoints;
    newPoints.reserve(snapshot.rowCount - cache.processedRowCount);

    auto convertResult =
        convertRowsToPoints(newPoints, series, resolved, snapshot, context,
                            cache.processedRowCount);
    if (!convertResult) {
      CP_WARN(convertResult.error().toStdString());
      return data;
    }

    if (data->dataIsSortedByX) {
      // Sort only the new delta points
      std::sort(
          newPoints.begin(), newPoints.end(),
          [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });

      // Merge sort the newly sorted delta with the already sorted cache
      if (cache.points.isEmpty()) {
        cache.points = std::move(newPoints);
      } else {
        QVector<QPointF> merged;
        merged.reserve(cache.points.size() + newPoints.size());
        std::merge(
            cache.points.begin(), cache.points.end(), newPoints.begin(),
            newPoints.end(), std::back_inserter(merged),
            [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });

        cache.points = std::move(merged);
      }
    } else {
      // No sorting required, simply append the new points
      cache.points.append(newPoints);
    }

    cache.processedRowCount = snapshot.rowCount;
  }

  // O(1) Copy utilizing Qt's implicit sharing
  data->points = cache.points;

  if (!data->points.isEmpty()) {
    // CP_DEBUG("calculating bound...");
    auto bound = calculateBound(data->points, context);
    data->points.assign(bound.first, bound.second);
  }

  // if (context.dataDownsampler && context.preferredTotalPoints > 0 &&
  //     data->points.size() > context.preferredTotalPoints) {
  //   // CP_DEBUG("down sampling data...");
  //   QVector<QPointF> originalPoints = std::move(data->points);
  //   data->points.resize(context.preferredTotalPoints);
  //
  //   context.dataDownsampler->downsample(
  //       originalPoints.begin(), originalPoints.size(), data->points.begin(),
  //       context.preferredTotalPoints);
  // }

  // CP_DEBUG("calculating data range...");
  for (const auto &p : data->points) {
    DataRange::includeValue(data->xRange, p.x());
    DataRange::includeValue(data->yRange, p.y());
  }

  // CP_DEBUG("finished!!!!!!");
  data->valid = true;
  return data;
}

std::expected<void, QString> LineSeriesStrategy::convertRowsToPoints(
    QVector<QPointF> &destination, const AbstractSeries &series,
    const ResolvedSeriesData &resolved, const DataSnapshot &snapshot,
    const SeriesBuildContext &context, qsizetype fromRow) const {
  const int xIndex = resolved.xColumnIndex;
  const int yIndex = resolved.yColumnIndex;
  const ChartEnums::DataType xType = resolved.xColumnType;

  if (xType != ChartEnums::DataType::Number &&
      xType != ChartEnums::DataType::Date &&
      xType != ChartEnums::DataType::String) {
    return std::unexpected(
        "LineSeriesStrategy::build: unsupported x column type");
  }

  for (qsizetype row = fromRow; row < snapshot.rowCount; ++row) {
    double x = snapshot.valueAt(xIndex, row);
    double y = snapshot.valueAt(yIndex, row);

    if (std::isnan(x) || std::isnan(y)) {
      continue;
    }

    if (xType == ChartEnums::DataType::String) {
      int localId = static_cast<int>(x);

      if (localId < 0 || localId >= resolved.localToGlobalXMap.size()) {
        continue;
      }

      x = resolved.localToGlobalXMap[localId];
    }

    destination.append(QPointF(x, y));
  }

  return {};
}

std::expected<void, QString>
LineSeriesStrategy::loadSeriesConfig(LineRenderData *data,
                                     const AbstractSeries &series,
                                     const SeriesBuildContext &context) const {
  assert(data != nullptr);

  const auto *lineSeries = qobject_cast<const LineSeries *>(&series);

  if (!lineSeries) {
    return std::unexpected(
        "LineSeriesStrategy::build: series is not LineSeries");
  }

  const float width = lineSeries->useGlobalStrokeWidth()
                          ? context.globalLineWidth
                          : lineSeries->strokeWidth();
  const float aa = lineSeries->useGlobalAntialias() ? context.globalAntialiasing
                                                    : lineSeries->antialias();

  data->marker.color = lineSeries->markerColor();
  data->marker.visible = lineSeries->markerVisible();
  data->stroke.color = lineSeries->strokeColor();
  data->stroke.width = width;
  data->stroke.pattern = lineSeries->strokePattern();
  data->stroke.dashStyle = DashStyle{
      .length = std::max(15.0f, width * 3.0f),
      .gap = std::max(10.0f, width * 2.0f),
  };
  data->stroke.dotStyle = DotStyle{
      .gap = std::max(10.0f, width),
  };
  data->antialias = aa;

  return {};
}

std::pair<QVector<QPointF>::const_iterator, QVector<QPointF>::const_iterator>
LineSeriesStrategy::calculateBound(const QVector<QPointF> points,
                                   const SeriesBuildContext &context) const {
  const double viewMin = context.viewportXRange.min;
  const double viewMax = context.viewportXRange.max;

  // Find the first point that is >= viewMin
  auto leftIt = std::lower_bound(
      points.begin(), points.end(), viewMin,
      [](const QPointF &p, double val) { return p.x() < val; });
  if (leftIt != points.begin()) {
    --leftIt;
  }

  auto rightIt = std::upper_bound(
      leftIt, points.end(), viewMax,
      [](double val, const QPointF &p) { return val < p.x(); });
  if (rightIt != points.end()) {
    ++rightIt;
  }

  return {leftIt, rightIt};
}

std::expected<void, QString>
LineSeriesStrategy::validateTyAndColIndex(const ResolvedSeriesData &resolved,
                                          const DataSnapshot &snapshot) const {
  const int xIndex = resolved.xColumnIndex;
  const int yIndex = resolved.yColumnIndex;
  const ChartEnums::DataType xType = resolved.xColumnType;
  const ChartEnums::DataType yType = resolved.yColumnType;

  if (!resolved.valid) {
    return std::unexpected(
        QString("LineSeriesStrategy::build: resolved series is invalid: %1")
            .arg(resolved.errorMessage));
  }

  if (xIndex < 0 || xIndex >= snapshot.columnCount) {
    return std::unexpected(
        QString("LineSeriesStrategy::build: invalid x column index %1")
            .arg(xIndex));
  }

  if (yIndex < 0 || yIndex >= snapshot.columnCount) {
    return std::unexpected(
        QString("LineSeriesStrategy::build: invalid y column index %1")
            .arg(yIndex));
  }

  if (yType != ChartEnums::DataType::Number) {
    return std::unexpected(
        "LineSeriesStrategy::build: y column must be Number");
  }

  return {};
}

} // namespace ChartPlotter

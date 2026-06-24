#include "ChartPlotter/strategy/LineSeriesStrategy.hpp"

#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/series/LineSeries.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "ChartPlotter/utils/Variant.hpp"

#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QPointF>
#include <QVariant>
#include <QVector>

#include <algorithm>
#include <memory>

namespace ChartPlotter {

std::unique_ptr<RenderData> LineSeriesStrategy::build(
    const AbstractSeries &series, const ResolvedSeriesData &resolved,
    const DataSnapshot &snapshot, const SeriesBuildContext &context) {
  auto data = std::make_unique<LineRenderData>();

  const int xIndex = resolved.xColumnIndex;
  const int yIndex = resolved.yColumnIndex;
  const ChartEnums::DataType xType = resolved.xColumnType;
  const ChartEnums::DataType yType = resolved.yColumnType;

  if (!resolved.valid) {
    CP_WARN("LineSeriesStrategy::build: resolved series is invalid: {}",
            resolved.errorMessage.toStdString());
    return data;
  }

  if (xIndex < 0 || xIndex >= snapshot.columnCount) {
    CP_WARN("LineSeriesStrategy::build: invalid x column index {}", xIndex);
    return data;
  }

  if (yIndex < 0 || yIndex >= snapshot.columnCount) {
    CP_WARN("LineSeriesStrategy::build: invalid y column index {}", yIndex);
    return data;
  }

  if (yType != ChartEnums::DataType::Number) {
    CP_WARN("LineSeriesStrategy::build: y column must be Number");
    return data;
  }

  auto result = loadSeriesConfig(data.get(), series, context);
  if (!result) {
    CP_WARN(result.error().toStdString());
    return data;
  }

  CP_DEBUG("converting rows to points...");
  data->points.resize(snapshot.rowCount);
  result = convertRowsToPoints(data->points.begin(), series, resolved, snapshot,
                               context, 0);
  if (!result) {
    CP_WARN(result.error().toStdString());
    return data;
  }
  std::sort(data->points.begin(), data->points.end(),
            [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });

  CP_DEBUG("calculating bound...");
  auto bound = calculateBound(data->points, context);
  data->points.assign(bound.first, bound.second);

  if (context.dataDownsampler && context.preferredTotalPoints > 0 &&
      data->points.size() > context.preferredTotalPoints) {
    CP_DEBUG("down sampling data...");
    QVector<QPointF> points;
    points.reserve(data->points.size());
    points.append(data->points);

    data->points.resize(context.preferredTotalPoints);
    context.dataDownsampler->downsample(points.begin(), points.size(),
                                        data->points.begin(),
                                        context.preferredTotalPoints);
  }

  CP_DEBUG("calculating data range...");
  for (const auto &p : data->points) {
    DataRange::includeValue(data->xRange, p.x());
    DataRange::includeValue(data->yRange, p.y());
  }

  CP_DEBUG("finished!!!!!!");
  data->valid = true;
  // CP_DEBUG(data->toString().toStdString());
  return data;
}

std::expected<void, QString> LineSeriesStrategy::convertRowsToPoints(
    QVector<QPointF>::iterator destinationIt, const AbstractSeries &series,
    const ResolvedSeriesData &resolved, const DataSnapshot &snapshot,
    const SeriesBuildContext &context, qsizetype fromRow) const {
  const int xIndex = resolved.xColumnIndex;
  const int yIndex = resolved.yColumnIndex;
  const ChartEnums::DataType xType = resolved.xColumnType;
  const ChartEnums::DataType yType = resolved.yColumnType;

  for (qsizetype row = fromRow; row < snapshot.rowCount; ++row) {
    const QVariant xValue = snapshot.valueAt(xIndex, row);
    const QVariant yValue = snapshot.valueAt(yIndex, row);

    if (!xValue.isValid() || xValue.isNull() || !yValue.isValid() ||
        yValue.isNull()) {
      continue;
    }

    double x = 0.0;
    double y = 0.0;

    switch (xType) {
    case ChartEnums::DataType::Number:
      if (!Utils::Variant::variantToDouble(xValue, x)) {
        continue;
      }
      break;

    case ChartEnums::DataType::Date:
      if (!Utils::Variant::variantToDateNumber(xValue, x)) {
        continue;
      }
      break;

    case ChartEnums::DataType::String: {
      if (!context.xCategories) {
        return std::unexpected(
            "LineSeriesStrategy::build: categorical x but no category axis");
      }
      const int idx = context.xCategories->indexOf(xValue.toString());
      if (idx < 0) {
        continue; // value wasn't in the shared axis (shouldn't happen)
      }
      x = static_cast<double>(idx);
      break;
    }

    default:
      return std::unexpected(
          "LineSeriesStrategy::build: unsupported x column type");
    }

    if (!Utils::Variant::variantToDouble(yValue, y)) {
      continue;
    }

    *destinationIt = QPointF(x, y);
    ++destinationIt;
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

} // namespace ChartPlotter

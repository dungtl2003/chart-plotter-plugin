#include "ChartPlotter/strategy/LineSeriesStrategy.hpp"

#include "ChartPlotter/constants/ChartConstants.hpp"
#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/data/LineRenderData.hpp"
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

  const auto *lineSeries = qobject_cast<const LineSeries *>(&series);

  if (!lineSeries) {
    CP_WARN("LineSeriesStrategy::build: series is not LineSeries");
    return data;
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

  if (!resolved.valid) {
    CP_WARN("LineSeriesStrategy::build: resolved series is invalid: {}",
            resolved.errorMessage.toStdString());
    return data;
  }

  const int xIndex = resolved.xColumnIndex;
  const int yIndex = resolved.yColumnIndex;

  if (xIndex < 0 || xIndex >= snapshot.columnCount) {
    CP_WARN("LineSeriesStrategy::build: invalid x column index {}", xIndex);
    return data;
  }

  if (yIndex < 0 || yIndex >= snapshot.columnCount) {
    CP_WARN("LineSeriesStrategy::build: invalid y column index {}", yIndex);
    return data;
  }

  const ChartEnums::DataType xType = resolved.xColumnType;
  const ChartEnums::DataType yType = resolved.yColumnType;

  if (yType != ChartEnums::DataType::Number) {
    CP_WARN("LineSeriesStrategy::build: y column must be Number");
    return data;
  }

  data->points.reserve(snapshot.rowCount);

  // CP_DEBUG(QString("xIndex = %1, yIndex = %2")
  //              .arg(xIndex)
  //              .arg(yIndex)
  //              .toStdString());

  QSet<double> hasX;
  for (int row = 0; row < snapshot.rowCount; ++row) {
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
        CP_WARN(
            "LineSeriesStrategy::build: categorical x but no category axis");
        return data;
      }
      const int idx = context.xCategories->indexOf(xValue.toString());
      if (idx < 0) {
        continue; // value wasn't in the shared axis (shouldn't happen)
      }
      x = static_cast<double>(idx);
      break;
    }

    default:
      CP_WARN("LineSeriesStrategy::build: unsupported x column type");
      return data;
    }

    if (!Utils::Variant::variantToDouble(yValue, y)) {
      continue;
    }

    // we don't accept multiple y, one x
    if (hasX.contains(x)) {
      continue;
    }

    data->points.push_back(QPointF(x, y));
    hasX.insert(x);
  }

  std::sort(data->points.begin(), data->points.end(),
            [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });

  const double viewMin = context.viewportXRange.min;
  const double viewMax = context.viewportXRange.max;

  // Find the first point that is >= viewMin
  auto leftIt = std::lower_bound(
      data->points.begin(), data->points.end(), viewMin,
      [](const QPointF &p, double val) { return p.x() < val; });
  if (leftIt != data->points.begin()) {
    --leftIt;
  }

  auto rightIt = std::upper_bound(
      leftIt, data->points.end(), viewMax,
      [](double val, const QPointF &p) { return val < p.x(); });
  if (rightIt != data->points.end()) {
    ++rightIt;
  }

  data->points.assign(leftIt, rightIt);
  for (const auto &p : data->points) {
    DataRange::includeValue(data->xRange, p.x());
    DataRange::includeValue(data->yRange, p.y());
  }
  data->valid = true;
  // CP_DEBUG(data->toString().toStdString());
  return data;
}

} // namespace ChartPlotter

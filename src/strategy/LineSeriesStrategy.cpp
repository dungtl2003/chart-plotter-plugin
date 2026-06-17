#include "ChartPlotter/strategy/LineSeriesStrategy.hpp"

#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/data/LineRenderData.hpp"
#include "ChartPlotter/series/LineSeries.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QPointF>
#include <QVariant>
#include <QVector>

#include <cmath>
#include <memory>

namespace ChartPlotter {

namespace {

void includeValue(DataRange &range, double value) {
  if (!std::isfinite(value)) {
    return;
  }

  if (!range.valid) {
    range.min = value;
    range.max = value;
    range.valid = true;
    return;
  }

  range.min = std::min(range.min, value);
  range.max = std::max(range.max, value);
}

bool variantToDouble(const QVariant &value, double &out) {
  bool ok = false;
  const double result = value.toDouble(&ok);

  if (!ok || !std::isfinite(result)) {
    return false;
  }

  out = result;
  return true;
}

bool variantToDateNumber(const QVariant &value, double &out) {
  if (value.canConvert<QDateTime>()) {
    const QDateTime dt = value.toDateTime();

    if (dt.isValid()) {
      out = static_cast<double>(dt.toMSecsSinceEpoch());
      return true;
    }
  }

  if (value.canConvert<QDate>()) {
    const QDate date = value.toDate();

    if (date.isValid()) {
      out =
          static_cast<double>(QDateTime(date.startOfDay()).toMSecsSinceEpoch());
      return true;
    }
  }

  return variantToDouble(value, out);
}

} // namespace

std::unique_ptr<RenderData> LineSeriesStrategy::build(
    const AbstractSeries &series, const ResolvedSeriesData &resolved,
    const DataSnapshot &snapshot, const SeriesBuildContext &context) {
  auto data = std::make_unique<LineRenderData>();

  const auto *lineSeries = qobject_cast<const LineSeries *>(&series);

  if (!lineSeries) {
    CP_WARN("LineSeriesStrategy::build: series is not LineSeries");
    return data;
  }

  data->marker.color = lineSeries->markerColor();
  data->marker.visible = lineSeries->markerVisible();
  data->stroke.color = lineSeries->strokeColor();
  data->stroke.width = lineSeries->strokeWidth();
  data->stroke.pattern = lineSeries->strokePattern();
  data->stroke.dashStyle = DashStyle{
      .length = data->stroke.width * 2.5f,
      .gap = data->stroke.width * 1.5f,
  };
  data->stroke.dotStyle = DotStyle{
      .gap = data->stroke.width,
  };
  data->antialias = lineSeries->antialias();

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
      if (!variantToDouble(xValue, x)) {
        continue;
      }
      break;

    case ChartEnums::DataType::Date:
      if (!variantToDateNumber(xValue, x)) {
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

    if (!variantToDouble(yValue, y)) {
      continue;
    }

    // we don't accept multiple y, one x
    if (hasX.contains(x)) {
      continue;
    }

    data->points.push_back(QPointF(x, y));
    hasX.insert(x);

    includeValue(data->xRange, x);
    includeValue(data->yRange, y);
  }

  std::sort(data->points.begin(), data->points.end(),
            [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });

  data->valid = true;
  // CP_DEBUG(data->toString().toStdString());
  return data;
}

} // namespace ChartPlotter

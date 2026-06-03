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

class CategoryMapper {
public:
  double valueFor(const QVariant &value) {
    const QString text = value.toString();

    if (!m_indexByName.contains(text)) {
      const int index = m_categories.size();
      m_categories.push_back(text);
      m_indexByName.insert(text, index);
    }

    return static_cast<double>(m_indexByName.value(text));
  }

private:
  QHash<QString, int> m_indexByName;
  QVector<QString> m_categories;
};

} // namespace

std::unique_ptr<RenderData>
LineSeriesStrategy::build(const AbstractSeries &series,
                          const ResolvedSeriesData &resolved,
                          const DataSnapshot &snapshot) {
  auto data = std::make_unique<LineRenderData>();

  const auto *lineSeries = qobject_cast<const LineSeries *>(&series);

  if (!lineSeries) {
    CP_WARN("LineSeriesStrategy::build: series is not LineSeries");
    return data;
  }

  data->marker.color = lineSeries->markerColor();
  data->marker.visible = lineSeries->markerVisible();
  data->marker.radius = lineSeries->markerRadius();
  data->stroke.color = lineSeries->strokeColor();
  data->stroke.width = lineSeries->strokeWidth();
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

  CategoryMapper categoryMapper;

  // CP_DEBUG(QString("xIndex = %1, yIndex = %2")
  //              .arg(xIndex)
  //              .arg(yIndex)
  //              .toStdString());

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

    case ChartEnums::DataType::String:
      x = categoryMapper.valueFor(xValue);
      break;

    default:
      CP_WARN("LineSeriesStrategy::build: unsupported x column type");
      return data;
    }

    if (!variantToDouble(yValue, y)) {
      continue;
    }

    data->points.push_back(QPointF(x, y));

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

#include "ChartPlotter/ChartLayoutPlanner.hpp"
#include "ChartPlotter/series/PieSeries.hpp"
#include "ChartPlotter/series/XYSeries.hpp"

namespace ChartPlotter {

ChartLayoutPlan
ChartLayoutPlanner::buildPlan(const QVector<QPointer<AbstractSeries>> &series) {
  ChartLayoutPlan plan;

  if (series.isEmpty()) {
    plan.valid = false;
    plan.errorMessage = "ChartView has no series";
    return plan;
  }

  for (int i = 0; i < series.size(); ++i) {
    const QPointer<AbstractSeries> current = series.at(i);

    if (!current) {
      plan.valid = false;
      plan.errorMessage = QString("Series at index %1 is null").arg(i);
      return plan;
    }

    const SeriesLayoutType layoutType = layoutTypeOf(current.data());

    switch (layoutType) {
    case SeriesLayoutType::XY:
      plan.xySeriesIndexes.push_back(i);
      break;

    case SeriesLayoutType::Pie:
      plan.pieSeriesIndexes.push_back(i);
      break;

    case SeriesLayoutType::Unknown:
    default:
      plan.valid = false;
      plan.errorMessage =
          QString("Series at index %1 has unsupported layout type").arg(i);
      return plan;
    }
  }

  const bool hasXY = !plan.xySeriesIndexes.isEmpty();
  const bool hasPie = !plan.pieSeriesIndexes.isEmpty();

  if (hasXY && hasPie) {
    plan.valid = false;
    plan.errorMessage =
        "Cannot mix XY series and Pie series in the same ChartView yet";
    return plan;
  }

  if (hasPie && plan.pieSeriesIndexes.size() > 1) {
    plan.valid = false;
    plan.errorMessage =
        "Multiple Pie series in one ChartView are not supported yet";
    return plan;
  }

  if (hasXY) {
    plan.valid = true;
    plan.layoutType = SeriesLayoutType::XY;
    return plan;
  }

  if (hasPie) {
    plan.valid = true;
    plan.layoutType = SeriesLayoutType::Pie;
    return plan;
  }

  plan.valid = false;
  plan.errorMessage = "ChartView has no supported series";
  return plan;
}

SeriesLayoutType
ChartLayoutPlanner::layoutTypeOf(const AbstractSeries *series) {
  if (!series) {
    return SeriesLayoutType::Unknown;
  }

  if (qobject_cast<const XYSeries *>(series)) {
    return SeriesLayoutType::XY;
  }

  if (qobject_cast<const PieSeries *>(series)) {
    return SeriesLayoutType::Pie;
  }

  return SeriesLayoutType::Unknown;
}

} // namespace ChartPlotter

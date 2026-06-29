#include "ChartPlotter/ChartLayoutPlanner.hpp"
#include "ChartPlotter/series/BarSeries.hpp"
#include "ChartPlotter/series/LineSeries.hpp"
#include "ChartPlotter/series/PieSeries.hpp"
#include "ChartPlotter/series/XYSeries.hpp"

namespace ChartPlotter {

ChartLayoutPlanner::ChartLayoutPlanner() { applyDefaultPolicies(); }

ChartLayoutPlan
ChartLayoutPlanner::buildPlan(const QVector<QPointer<AbstractSeries>> &series,
                              const ChartChromeRequest &chrome) const {
  ChartLayoutPlan plan;

  for (const auto &policy : m_policies) {
    if (policy.get() == nullptr) {
      continue;
    }

    auto [isValid, errorMessage] = policy->apply(series);
    if (!isValid) {
      plan.valid = false;
      plan.errorMessage = std::move(errorMessage);
      return plan;
    }
  }

  for (int i = 0; i < series.size(); ++i) {
    const QPointer<AbstractSeries> current = series.at(i);

    if (!current) {
      plan.valid = false;
      plan.errorMessage = QString("Series at index %1 is null").arg(i);
      return plan;
    }

    const CoordinateSystem coordinateSystem =
        coordinateSystemOf(current.data());

    switch (coordinateSystem) {
    case CoordinateSystem::Cartesian:
      plan.xySeriesIndexes.push_back(i);
      break;

    case CoordinateSystem::Pie:
      plan.pieSeriesIndexes.push_back(i);
      break;

    case CoordinateSystem::Unknown:
    default:
      plan.valid = false;
      plan.errorMessage =
          QString("Series at index %1 has unsupported layout type").arg(i);
      return plan;
    }
  }

  const bool hasXY = !plan.xySeriesIndexes.isEmpty();
  const bool hasPie = !plan.pieSeriesIndexes.isEmpty();
  const bool hasSeries =
      !plan.xySeriesIndexes.isEmpty() || !plan.pieSeriesIndexes.isEmpty();
  plan.hasTitle = chrome.hasTitle && hasSeries;
  plan.legendPosition =
      hasSeries ? chrome.legendPosition : ChartEnums::LegendPosition::None;

  // TODO: we need to have strategy to find the good margin instead of these
  // magic numbers
  if (plan.hasTitle || plan.legendPosition == ChartEnums::LegendPosition::Top) {
    plan.plotMargins.top = 30;
  }
  if (plan.legendPosition == ChartEnums::LegendPosition::Left) {
    plan.plotMargins.left = 30;
  }
  if (plan.legendPosition == ChartEnums::LegendPosition::Bottom) {
    plan.plotMargins.bottom = 40;
  }
  if (plan.legendPosition == ChartEnums::LegendPosition::Right) {
    plan.plotMargins.right = 30;
  }

  if (hasXY) {
    plan.valid = true;
    plan.coordinateSystem = CoordinateSystem::Cartesian;
    return plan;
  }
  if (hasPie) {
    plan.valid = true;
    plan.coordinateSystem = CoordinateSystem::Pie;
    return plan;
  }

  plan.valid = false;
  plan.errorMessage = "ChartView has no supported series";
  return plan;
}

CoordinateSystem
ChartLayoutPlanner::coordinateSystemOf(const AbstractSeries *series) {
  if (!series) {
    return CoordinateSystem::Unknown;
  }

  if (qobject_cast<const XYSeries *>(series)) {
    return CoordinateSystem::Cartesian;
  }

  if (qobject_cast<const PieSeries *>(series)) {
    return CoordinateSystem::Pie;
  }

  return CoordinateSystem::Unknown;
}

void ChartLayoutPlanner::applyDefaultPolicies() {
  m_policies.clear();
  m_policies.reserve(5);
  m_policies.push_back(std::make_unique<SeriesNotEmptyPolicy>());
  m_policies.push_back(std::make_unique<XYAndPieMixNotAllowedPolicy>());
  m_policies.push_back(std::make_unique<MultipleBarSeriesNotAllowedPolicy>());
  m_policies.push_back(std::make_unique<MultiplePieSeriesNotAllowedPolicy>());
}

std::pair<bool, QString> XYAndPieMixNotAllowedPolicy::apply(
    const QVector<QPointer<AbstractSeries>> &series) const {
  bool hasXY = false;
  bool hasPie = false;

  for (const auto &series : series) {
    if (series.isNull()) {
      continue;
    }

    if (qobject_cast<const XYSeries *>(series)) {
      hasXY = true;
    }
    if (qobject_cast<const PieSeries *>(series)) {
      hasPie = true;
    }

    if (hasXY && hasPie) {
      return {
          false,
          "Mix XY series and Pie series in the same ChartView is not allowed"};
    }
  }

  return {true, ""};
}

std::pair<bool, QString> LineAndBarMixNotAllowedPolicy::apply(
    const QVector<QPointer<AbstractSeries>> &series) const {
  bool hasLine = false;
  bool hasBar = false;

  for (const auto &series : series) {
    if (series.isNull()) {
      continue;
    }

    if (qobject_cast<const LineSeries *>(series)) {
      hasLine = true;
    }
    if (qobject_cast<const BarSeries *>(series)) {
      hasBar = true;
    }

    if (hasLine && hasBar) {
      return {false, "Mix Line series and Bar series in the same ChartView is "
                     "not allowed"};
    }
  }

  return {true, ""};
}

std::pair<bool, QString> MultipleBarSeriesNotAllowedPolicy::apply(
    const QVector<QPointer<AbstractSeries>> &series) const {
  bool hasBar = false;

  for (const auto &series : series) {
    if (series.isNull()) {
      continue;
    }

    if (qobject_cast<const BarSeries *>(series)) {
      if (hasBar) {
        return {false, "Multiple Bar series in the same ChartView is "
                       "not allowed"};
      }
      hasBar = true;
    }
  }

  return {true, ""};
}

std::pair<bool, QString> MultiplePieSeriesNotAllowedPolicy::apply(
    const QVector<QPointer<AbstractSeries>> &series) const {
  bool hasPie = false;

  for (const auto &series : series) {
    if (series.isNull()) {
      continue;
    }

    if (qobject_cast<const PieSeries *>(series)) {
      if (hasPie) {
        return {false, "Multiple Pie series in the same ChartView is "
                       "not allowed"};
      }
      hasPie = true;
    }
  }

  return {true, ""};
}

std::pair<bool, QString> SeriesNotEmptyPolicy::apply(
    const QVector<QPointer<AbstractSeries>> &series) const {
  if (series.isEmpty()) {
    return {false, "ChartView must have series"};
  }

  return {true, ""};
}

} // namespace ChartPlotter

#include "ChartPlotter/axis/ValueAxis.hpp"
#include "ChartPlotter/constants/ChartConstants.hpp"
#include "ChartPlotter/utils/RenderMath.hpp"

#include <QDateTime>

namespace ChartPlotter {

AxisTicks ValueAxis::calculateTicks(const AxisRange &range, int targetTickCount,
                                    bool isDateTime) {
  AxisTicks result;

  if (targetTickCount < 2) {
    targetTickCount = 2;
  }

  double min = range.min;
  double max = range.max;

  if (!std::isfinite(min) || !std::isfinite(max) || min == max) {
    min = -1.0;
    max = 1.0;
  }

  const double rawRange = max - min;

  double niceRange = RenderMath::niceNumber(rawRange, false);
  double step = RenderMath::niceNumber(niceRange / (targetTickCount - 1), true);

  result.step = step;

  double tickMin = std::floor(min / step) * step;
  double tickMax = std::ceil(max / step) * step;
  // tickMax + step * 0.5 helps to avoid floating-point precision issues.
  // When add double repeately, the final result can be like: 99.99...9999 or
  // 100.000...01, so the final tick can be missed.
  for (double value = tickMin; value <= tickMax + step * 0.5; value += step) {
    AxisTick tick;
    tick.value = value;
    if (!isDateTime) {
      tick.label = formatTickLabel(value, step);
    } else {
      tick.label = QDateTime::fromMSecsSinceEpoch(value).toString("yyyy-MM-dd");
    }
    result.ticks.append(tick);
  }

  // double firstTick = std::ceil(min / step) * step;
  // double firstTick = std::floor(min / step) * step;
  // const double precisionEpsilon = step * ChartConstants::EPSILON;
  // for (double value = firstTick; value <= max + precisionEpsilon;
  //      value += step) {
  //   AxisTick tick;
  //   tick.value = value;
  //
  //   if (!isDateTime) {
  //     tick.label = formatTickLabel(value, step);
  //   } else {
  //     tick.label =
  //     QDateTime::fromMSecsSinceEpoch(value).toString("yyyy-MM-dd");
  //   }
  //
  //   result.ticks.append(tick);
  // }

  return result;
}

QString ValueAxis::formatTickLabel(double value, double step) {
  if (std::abs(value) <
      step * ChartConstants::EPSILON) { // floating-point prec issue guard
    value = 0.0;
  }

  int decimals = 0;

  if (step < 1.0) {
    decimals = static_cast<int>(std::ceil(-std::log10(step)));
    decimals = std::clamp(decimals, 0, 6);
  }

  return QString::number(value, 'f', decimals);
}

AxisRange ValueAxis::calculateRange(const DataRange &dataRange) {
  return AxisRange{.min = dataRange.min, .max = dataRange.max};
}

} // namespace ChartPlotter

#include "ChartPlotter/axis/ValueAxis.hpp"
#include "ChartPlotter/constants/ChartConstants.hpp"
#include "ChartPlotter/utils/RenderMath.hpp"

#include <QDateTime>
#include <QTimeZone>

namespace ChartPlotter {

AxisTicks ValueAxis::calculateTicks(const AxisRange &range, int targetTickCount,
                                    bool isDateTime) {
  AxisTicks result;

  const auto niceBound =
      RenderMath::expandToNiceBounds(range.toDataRange(), targetTickCount);
  if (!niceBound.range.valid) {
    return result;
  }

  double step = niceBound.step;
  double tickMin = niceBound.range.min;
  double tickMax = niceBound.range.max;
  result.step = step;

  // Defensive: a non-positive/non-finite step would make the loop below never
  // advance (infinite append → crash). expandToNiceBounds already guards this,
  // but bail here too rather than trust the input.
  if (!std::isfinite(step) || step <= 0.0) {
    return result;
  }

  // tickMax + step * 0.5 helps to avoid floating-point precision issues.
  // When add double repeately, the final result can be like: 99.99...9999 or
  // 100.000...01, so the final tick can be missed.
  for (double value = tickMin; value <= tickMax + step * 0.5; value += step) {
    // Hard cap so a pathological range can never spin an unbounded loop.
    if (result.ticks.size() >= ChartConstants::MAX_AXIS_TICKS) {
      break;
    }
    AxisTick tick;
    tick.value = value;
    if (!isDateTime) {
      tick.label = formatTickLabel(value, step);
    } else {
      tick.label = QDateTime::fromMSecsSinceEpoch(value, QTimeZone::UTC)
                       .toString("MM/dd/yyyy hh:mm:ss");
    }
    result.ticks.append(tick);
  }

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

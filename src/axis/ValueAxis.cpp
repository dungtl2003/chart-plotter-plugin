#include "ChartPlotter/axis/ValueAxis.hpp"

#include <QDateTime>

namespace ChartPlotter {

/**
 * It will try to round the value to one of these value:
 * 1 * 10^n
 * 2 * 10^n
 * 5 * 10^n
 * 10 * 10^n
 *
 * First, calculate exponent: 10^n = value -> n = log10(value)
 * Then calculate the fraction: fraction = value / 10^n
 * fraction can only in range (0, 10), so we can round (cell) fraction on that
 * range to find suitable nice number.
 *
 * nice number = nice fraction * 10^n
 */
double ValueAxis::niceNumber(double value, bool round) {
  double exponent = std::floor(std::log10(value));
  double fraction = value / std::pow(10.0, exponent);

  double niceFraction;

  if (round) {
    if (fraction < 1.5) {
      niceFraction = 1.0;
    } else if (fraction < 3.0) {
      niceFraction = 2.0;
    } else if (fraction < 7.0) {
      niceFraction = 5.0;
    } else {
      niceFraction = 10.0;
    }
  } else {
    if (fraction <= 1.0) {
      niceFraction = 1.0;
    } else if (fraction <= 2.0) {
      niceFraction = 2.0;
    } else if (fraction <= 5.0) {
      niceFraction = 5.0;
    } else {
      niceFraction = 10.0;
    }
  }

  return niceFraction * std::pow(10.0, exponent);
}

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

  double niceRange = niceNumber(rawRange, false);
  double step = niceNumber(niceRange / (targetTickCount - 1), true);

  double tickMin = std::floor(min / step) * step;
  double tickMax = std::ceil(max / step) * step;

  result.step = step;

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

  return result;
}

QString ValueAxis::formatTickLabel(double value, double step) {
  if (std::abs(value) < step * 1e-9) { // floating-point prec issue guard
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

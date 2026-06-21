#pragma once

#include "ChartPlotter/types/DataRange.hpp"

#include <QString>
#include <QVector>

namespace ChartPlotter {

struct AxisRange {
  double min;
  double max;

  static AxisRange fromDataRange(const DataRange &range) {
    return AxisRange{.min = range.min, .max = range.max};
  }
};

struct AxisTick {
  double value;
  QString label;
};

struct AxisTicks {
  double step;
  QVector<AxisTick> ticks;
};

struct AxisModel {
  AxisRange range;
  QVector<AxisTick> ticks;
};

} // namespace ChartPlotter

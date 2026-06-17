#pragma once

#include <QString>
#include <QVector>

namespace ChartPlotter {

struct AxisRange {
  double min;
  double max;
};

struct AxisTick {
  double value;
  QString label;
};

struct AxisTicks {
  double step;
  QVector<AxisTick> ticks;
};

} // namespace ChartPlotter

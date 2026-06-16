#pragma once

#include "ChartPlotter/data/DataRange.hpp"

#include <QString>
#include <QVector>

namespace ChartPlotter {

struct ValueAxisRange {
  double min;
  double max;
};

struct ValueAxisTick {
  double value;
  QString label;
};

struct ValueAxisTicks {
  double step;
  QVector<ValueAxisTick> ticks;
};

class ValueAxis {

public:
  static ValueAxisTicks calculateTicks(const ValueAxisRange &range,
                                       int targetTickCount = 6);
  static ValueAxisRange calculateRange(const DataRange &dataRange);

private:
  static double niceNumber(double value, bool round);
  static QString formatTickLabel(double value, double step);
};

} // namespace ChartPlotter

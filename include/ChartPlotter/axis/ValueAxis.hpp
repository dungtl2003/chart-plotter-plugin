#pragma once

#include "ChartPlotter/axis/Axis.hpp"
#include "ChartPlotter/data/DataRange.hpp"

#include <QString>
#include <QVector>

namespace ChartPlotter {

class ValueAxis {

public:
  static AxisTicks calculateTicks(const AxisRange &range,
                                  int targetTickCount = 6,
                                  bool isDateTime = false);
  static AxisRange calculateRange(const DataRange &dataRange);

private:
  static double niceNumber(double value, bool round);
  static QString formatTickLabel(double value, double step);
};

} // namespace ChartPlotter

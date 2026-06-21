#pragma once

#include <QHash>
#include <QString>
#include <QVector>

namespace ChartPlotter {

enum class AxisValueType { Numeric, DateTime, Category, Unknown };

struct DataRange {
  AxisValueType type = AxisValueType::Unknown;

  double min = 0.0;
  double max = 0.0;
  bool valid = false;

  QString toString() const {
    return QString("DataRange({min = %1, max = %2, valid = %3})")
        .arg(min)
        .arg(max)
        .arg(valid ? "true" : "false");
  };

  static void includeValue(DataRange &range, double value) {
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
};

} // namespace ChartPlotter

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

  QVector<QString> categories;
  QHash<QString, int> categoryIndex;

  static DataRange makeCategoryRange(const QVector<QVariant> &values);

  QString toString() const {
    return QString("DataRange({min = %1, max = %2, valid = %3})")
        .arg(min)
        .arg(max)
        .arg(valid ? "true" : "false");
  };
};

} // namespace ChartPlotter

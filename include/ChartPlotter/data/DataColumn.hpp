#pragma once

#include "ChartPlotter/types/ChartEnums.hpp"

#include <QString>
#include <QVariant>
#include <QVector>

namespace ChartPlotter {

struct DataColumn {
  QString name;
  ChartEnums::DataType type = ChartEnums::DataType::Unknown;
  QVector<QVariant> values;

  int size() const;
  bool isEmpty() const;
  QVariant valueAt(int row) const;
  void appendValue(const QVariant &value);
  void clearValues();
  QString toString() const;
};

} // namespace ChartPlotter

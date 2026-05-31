#pragma once

#include <QString>
#include <QVariant>
#include <QVector>

namespace ChartPlotter {

struct DataRow {
  QVector<QVariant> values;

  qsizetype size() const;
  bool isEmpty() const;
  QVariant value(qsizetype index) const;
  void clear();
  void append(const QVariant &value);
  std::string toString() const;
};

} // namespace ChartPlotter

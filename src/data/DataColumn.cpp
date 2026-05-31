#include "ChartPlotter/data/DataColumn.hpp"

#include <magic_enum/magic_enum.hpp>

namespace ChartPlotter {

int DataColumn::size() const { return values.size(); }

bool DataColumn::isEmpty() const { return values.isEmpty(); }

QVariant DataColumn::valueAt(int row) const {
  if (row < 0 || row >= values.size()) {
    return QVariant();
  }

  return values.at(row);
}

void DataColumn::appendValue(const QVariant &value) { values.push_back(value); }

void DataColumn::clearValues() { values.clear(); }

QString DataColumn::toString() const {
  QString valuesResult;
  valuesResult.reserve(values.size() * 17);

  for (size_t i = 0; i < values.size(); ++i) {
    valuesResult.append("\"" + values[i].toString() + "\"");
    if (i < values.size() - 1) {
      valuesResult.append(",");
    }
  }

  return "DataColumn({ name = " + name +
         ", type = " + QString(magic_enum::enum_name(type).data()) +
         ", values = [" + valuesResult + "]})";
}

} // namespace ChartPlotter

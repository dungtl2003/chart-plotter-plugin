#include "ChartPlotter/data/DataRow.hpp"

namespace ChartPlotter {

qsizetype DataRow::size() const { return values.size(); }

bool DataRow::isEmpty() const { return values.isEmpty(); }

QVariant DataRow::value(qsizetype index) const {
  if (index < 0 || index >= values.size()) {
    return QVariant();
  }

  return values.at(index);
}

void DataRow::clear() { values.clear(); }
void DataRow::append(const QVariant &value) { values.push_back(value); }
std::string DataRow::toString() const {
  if (values.empty()) {
    return "DataRow()";
  }

  std::string result;
  result.reserve(values.size() * 16);

  for (size_t i = 0; i < values.size(); ++i) {
    result.append("\"" + values[i].toString().toStdString() + "\"");
    if (i < values.size() - 1) {
      result.append(",");
    }
  }

  return "DataRow(" + result + ")";
}

} // namespace ChartPlotter

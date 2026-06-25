#include "ChartPlotter/utils/DataRangeCalculator.hpp"

namespace ChartPlotter {

DataRange
DataRangeCalculator::calculateColumnRange(const DataSnapshot &snapshot,
                                          qint64 columnIndex) {

  DataRange range;

  if (columnIndex < 0 || columnIndex >= snapshot.columnCount) {
    return range;
  }

  for (qint64 row = 0; row < snapshot.rowCount; ++row) {
    double numValue = snapshot.valueAt(columnIndex, row);

    if (!std::isnan(numValue) && std::isfinite(numValue)) {
      DataRange::includeValue(range, numValue);
    }
  }

  return range;
}

DataRange
DataRangeCalculator::calculateColumnRange(const DataSnapshot &snapshot,
                                          int columnIndex, quint64 fromRow) {

  DataRange range;

  if (columnIndex < 0 || columnIndex >= snapshot.columnCount) {
    return range;
  }

  for (quint64 row = fromRow; row < snapshot.rowCount; ++row) {
    double numValue = snapshot.valueAt(columnIndex, row);

    if (!std::isnan(numValue) && std::isfinite(numValue)) {
      DataRange::includeValue(range, numValue);
    }
  }

  return range;
}

DataRange DataRangeCalculator::unionRange(const DataRange &a,
                                          const DataRange &b) {
  if (!a.valid) {
    return b;
  }

  if (!b.valid) {
    return a;
  }

  DataRange result = a;
  result.min = std::min(a.min, b.min);
  result.max = std::max(a.max, b.max);
  result.valid = true;

  return result;
}

} // namespace ChartPlotter

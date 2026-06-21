#include "ChartPlotter/utils/DataRangeCalculator.hpp"
#include "ChartPlotter/utils/Variant.hpp"

namespace ChartPlotter {

DataRange DataRangeCalculator::calculateColumnRange(
    const DataSnapshot &snapshot, int columnIndex,
    ChartEnums::DataType dataType, const CategoryAxis *categories) {
  DataRange range;

  if (columnIndex < 0 || columnIndex >= snapshot.columnCount) {
    return range;
  }

  for (int row = 0; row < snapshot.rowCount; ++row) {
    const QVariant value = snapshot.valueAt(columnIndex, row);
    if (!value.isValid() || value.isNull()) {
      continue;
    }

    double numValue = 0.0;
    bool converted = false;

    switch (dataType) {
    case ChartEnums::DataType::Number:
      converted = Utils::Variant::variantToDouble(value, numValue);
      break;
    case ChartEnums::DataType::Date:
      converted = Utils::Variant::variantToDateNumber(value, numValue);
      break;
    case ChartEnums::DataType::String: {
      if (categories) {
        const int idx = categories->indexOf(value.toString());
        if (idx >= 0) {
          numValue = static_cast<double>(idx);
          converted = true;
        }
      }
      break;
    }
    default:
      continue;
    }

    if (converted && std::isfinite(numValue)) {
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

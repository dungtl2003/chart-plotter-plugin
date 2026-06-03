#include "ChartPlotter/data/DataRange.hpp"

#include <QVariant>

namespace ChartPlotter {

DataRange DataRange::makeCategoryRange(const QVector<QVariant> &values) {
  DataRange range;
  range.type = AxisValueType::Category;

  for (const QVariant &value : values) {
    const QString text = value.toString();

    if (text.isEmpty()) {
      continue;
    }

    if (range.categoryIndex.contains(text)) {
      continue;
    }

    const int index = range.categories.size();

    range.categories.push_back(text);
    range.categoryIndex.insert(std::move(text), index);
  }

  if (!range.categories.isEmpty()) {
    range.min = 0.0;
    range.max = static_cast<double>(range.categories.size() - 1);
    range.valid = true;
  }

  return range;
}

} // namespace ChartPlotter

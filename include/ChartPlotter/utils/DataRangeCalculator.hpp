#pragma once

#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/types/DataRange.hpp"

namespace ChartPlotter {

class DataRangeCalculator {
public:
  static DataRange calculateColumnRange(const DataSnapshot &snapshot,
                                        qint64 columnIndex);

  static DataRange unionRange(const DataRange &a, const DataRange &b);
};

} // namespace ChartPlotter

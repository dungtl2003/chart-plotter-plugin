#pragma once

#include "ChartPlotter/axis/CategoryAxis.hpp"
#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"
#include "ChartPlotter/types/DataRange.hpp"

namespace ChartPlotter {

class DataRangeCalculator {
public:
  static DataRange calculateColumnRange(const DataSnapshot &snapshot,
                                        int columnIndex,
                                        ChartEnums::DataType dataType,
                                        const CategoryAxis *categories);

  static DataRange unionRange(const DataRange &a, const DataRange &b);
};

} // namespace ChartPlotter

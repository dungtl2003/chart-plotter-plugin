#pragma once

#include "ChartPlotter/axis/CategoryAxis.hpp"
#include "ChartPlotter/data/AxisRenderData.hpp"

namespace ChartPlotter {

class AxisBuilder {
public:
  static AxisModel buildValueAxis(const DataRange &dataRange,
                                  bool isDate = false,
                                  std::optional<int> tickCount = std::nullopt);

  static AxisModel buildCategoryAxis(const CategoryAxis &categories);

  static std::unique_ptr<AxisRenderData>
  toRenderData(const AxisModel &model, ChartEnums::AxisPosition pos,
               double baseline);
};

} // namespace ChartPlotter

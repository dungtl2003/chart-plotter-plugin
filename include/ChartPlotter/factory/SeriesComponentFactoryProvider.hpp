#pragma once

#include "ChartPlotter/factory/ISeriesComponentFactory.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"

namespace ChartPlotter {

class SeriesComponentFactoryProvider {
public:
  static std::unique_ptr<ISeriesComponentFactory>
  getFactory(ChartEnums::SeriesType type);
};

} // namespace ChartPlotter

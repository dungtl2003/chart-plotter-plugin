#include "ChartPlotter/chart/AbstractChart.hpp"
#include "ChartPlotter/factory/IChartComponentFactory.hpp"

#include <memory>

class ChartComponentFactoryProvider {
public:
  static std::unique_ptr<IChartComponentFactory> getFactory(ChartType type);
};

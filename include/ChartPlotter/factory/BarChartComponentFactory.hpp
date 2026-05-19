#include "ChartPlotter/factory/IChartComponentFactory.hpp"

#include <memory>

class BarChartComponentFactory : public IChartComponentFactory {
public:
  std::unique_ptr<IChartStrategy> getStrategy() const override;
  std::unique_ptr<IChartRenderer> getRenderer() const override;
};

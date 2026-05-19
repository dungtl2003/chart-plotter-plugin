#include "IChartComponentFactory.hpp"

#include <memory>

class LineChartComponentFactory : public IChartComponentFactory {
public:
  std::unique_ptr<IChartStrategy> getStrategy() const override;
  std::unique_ptr<IChartRenderer> getRenderer() const override;
};

#include "ChartPlotter/strategy/IChartStrategy.hpp"

class BarChartStrategy : public IChartStrategy {
public:
  void calculate() override;
};

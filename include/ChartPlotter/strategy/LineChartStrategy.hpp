#include "ChartPlotter/strategy/IChartStrategy.hpp"

class LineChartStrategy : public IChartStrategy {
public:
  void calculate() override;
};

#pragma once

class IChartStrategy {
public:
  IChartStrategy() = default;

  explicit IChartStrategy(const IChartStrategy &) = delete;
  explicit IChartStrategy(IChartStrategy &&) = delete;
  IChartStrategy &operator=(const IChartStrategy &) = delete;
  IChartStrategy &operator=(IChartStrategy &&) = delete;
  virtual ~IChartStrategy() = default;

  virtual void calculate() = 0;
};

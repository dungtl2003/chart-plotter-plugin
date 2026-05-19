#pragma once

class IChartRenderer {
public:
  IChartRenderer() = default;

  explicit IChartRenderer(const IChartRenderer &) = delete;
  explicit IChartRenderer(IChartRenderer &&) = delete;
  IChartRenderer &operator=(const IChartRenderer &) = delete;
  IChartRenderer &operator=(IChartRenderer &&) = delete;
  virtual ~IChartRenderer() = default;

  virtual void render() = 0;
};

#pragma once

#include "ChartPlotter/renderer/IChartRenderer.hpp"
#include "ChartPlotter/strategy/IChartStrategy.hpp"

#include <memory>

class IChartComponentFactory {
public:
  IChartComponentFactory() = default;

  explicit IChartComponentFactory(const IChartComponentFactory &) = delete;
  explicit IChartComponentFactory(IChartComponentFactory &&) = delete;
  IChartComponentFactory &operator=(const IChartComponentFactory &) = delete;
  IChartComponentFactory &operator=(IChartComponentFactory &&) = delete;
  virtual ~IChartComponentFactory() = default;

  virtual std::unique_ptr<IChartStrategy> getStrategy() const = 0;
  virtual std::unique_ptr<IChartRenderer> getRenderer() const = 0;
};

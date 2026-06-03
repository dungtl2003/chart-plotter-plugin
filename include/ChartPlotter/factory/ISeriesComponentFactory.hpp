#pragma once

#include "ChartPlotter/renderer/IOpenGLRenderer.hpp"
#include "ChartPlotter/strategy/ISeriesStrategy.hpp"

#include <memory>

namespace ChartPlotter {

class ISeriesComponentFactory {
public:
  ISeriesComponentFactory() = default;

  explicit ISeriesComponentFactory(const ISeriesComponentFactory &) = delete;
  explicit ISeriesComponentFactory(ISeriesComponentFactory &&) = delete;
  ISeriesComponentFactory &operator=(const ISeriesComponentFactory &) = delete;
  ISeriesComponentFactory &operator=(ISeriesComponentFactory &&) = delete;
  virtual ~ISeriesComponentFactory() = default;

  virtual std::unique_ptr<ISeriesStrategy> getStrategy() const = 0;
  virtual std::unique_ptr<IOpenGLRenderer> getRenderer() const = 0;
};

} // namespace ChartPlotter

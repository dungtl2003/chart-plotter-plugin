#pragma once

#include "ChartPlotter/factory/ISeriesComponentFactory.hpp"

#include <memory>

namespace ChartPlotter {

class BarSeriesComponentFactory : public ISeriesComponentFactory {
public:
  std::unique_ptr<ISeriesStrategy> getStrategy() const override;
  std::unique_ptr<IOpenGLRenderer> getRenderer() const override;
};

} // namespace ChartPlotter

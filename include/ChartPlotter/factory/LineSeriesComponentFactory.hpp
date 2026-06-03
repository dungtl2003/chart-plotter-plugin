#pragma once

#include "ISeriesComponentFactory.hpp"

#include <memory>

namespace ChartPlotter {

class LineSeriesComponentFactory : public ISeriesComponentFactory {
public:
  std::unique_ptr<ISeriesStrategy> getStrategy() const override;
  std::unique_ptr<IOpenGLRenderer> getRenderer() const override;
};

} // namespace ChartPlotter

#pragma once

#include "ChartPlotter/data/RenderData.hpp"

#include <QColor>

namespace ChartPlotter {

struct BarRenderData : public XYSeriesRenderData {
  QColor color = QColor("#3d7eff");
  // Width of one bar's slot in data units (1.0 for a category axis, the
  // smallest gap between neighbouring x values for a numeric axis). The
  // renderer maps this to pixels against the current axis range every frame.
  double bandWidth = 1.0;
  double baseline = 0.0;
};

} // namespace ChartPlotter

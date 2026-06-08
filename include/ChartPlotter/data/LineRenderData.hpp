#pragma once

#include "ChartPlotter/data/RenderData.hpp"

#include <QColor>

namespace ChartPlotter {

struct Stroke {
  QColor color = QColor("#ff3333");
  float width = 1.0f;
};

struct Marker {
  bool visible = false;
  QColor color = QColor("#000000");
  float radius = 2.0f;
};

struct LineRenderData : public XYSeriesRenderData {
  Stroke stroke;
  Marker marker;

  float antialias = 1.0f;
};

} // namespace ChartPlotter

#pragma once

#include "ChartPlotter/data/RenderData.hpp"

#include <QColor>

namespace ChartPlotter {

struct Stroke {
  QColor color = QColor("#ff3333");
  int width = 1;
};

struct Marker {
  bool visible = false;
  QColor color = QColor("#000000");
  int radius = 1;
};

struct LineRenderData : public XYSeriesRenderData {
  Stroke stroke;
  Marker marker;

  int antialias = 1;
};

} // namespace ChartPlotter

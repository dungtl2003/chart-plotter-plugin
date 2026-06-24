#pragma once

#include "ChartPlotter/data/RenderData.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"

#include <QColor>

namespace ChartPlotter {

struct DotStyle {
  float gap = 5.0f;
};

struct DashStyle {
  float length = 5.0f;
  float gap = 5.0f;
};

struct Stroke {
  QColor color = QColor("#ff3333");
  float width = 5.0f;
  ChartEnums::StrokePattern pattern = ChartEnums::StrokePattern::Solid;
  DashStyle dashStyle;
  DotStyle dotStyle;
};

struct Marker {
  bool visible = false;
  QColor color = QColor("#000000");
};

struct LineRenderData : public XYSeriesRenderData {
  Stroke stroke;
  Marker marker;

  float antialias = 1.0f;
  bool dataIsSortedByX = false;
};

} // namespace ChartPlotter

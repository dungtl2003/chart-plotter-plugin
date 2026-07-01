#pragma once

#include "ChartPlotter/data/RenderData.hpp"

#include <QColor>
#include <QString>
#include <QVector>

namespace ChartPlotter {

// One wedge of the pie. Angles are in radians, measured clockwise from the top
// (12 o'clock) — the conventional pie orientation. The renderer tessellates
// [startAngle, startAngle + sweepAngle] into a triangle fan.
struct PieSlice {
  double startAngle = 0.0;
  double sweepAngle = 0.0;
  QColor color;
  QString label;
  double value = 0.0;
  double percentage = 0.0; // share of the total, in [0, 100]
};

struct PieRenderData : public RenderData {
  QVector<PieSlice> slices;

  QString toString() const override {
    return QString("PieRenderData(isValid = %1, slices = %2)")
        .arg(valid ? "true" : "false")
        .arg(slices.size());
  }
};

} // namespace ChartPlotter

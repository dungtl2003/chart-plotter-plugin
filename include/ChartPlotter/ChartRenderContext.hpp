#pragma once

#include "ChartPlotter/axis/Axis.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"
#include "ChartPlotter/types/DataRange.hpp"

#include <QMatrix4x4>
#include <QOpenGLExtraFunctions>

namespace ChartPlotter {

struct ChartRenderContext {
  QOpenGLExtraFunctions *f;
  QMatrix4x4 mvp;

  QRectF itemRect;
  QRectF plotArea;

  DataRange xRange;
  DataRange yRange;

  AxisRange xAxisRange;
  AxisRange yAxisRange;

  ChartEnums::AxisPositions axisPositions;
};

} // namespace ChartPlotter

#pragma once

#include "ChartPlotter/axis/Axis.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"
#include "ChartPlotter/types/DataRange.hpp"

#include <QRectF>

namespace ChartPlotter {

struct RenderData {
  RenderData() = default;

  explicit RenderData(const RenderData &) = delete;
  explicit RenderData(RenderData &&) = delete;
  RenderData &operator=(const RenderData &) = delete;
  RenderData &operator=(RenderData &&) = delete;
  virtual ~RenderData() = default;

  virtual QString toString() const {
    return QString("RenderData(isValid = %1)").arg(valid);
  }

  bool valid = false;
};

struct XYSeriesRenderData : public RenderData {
  QVector<QPointF> points;
  DataRange xRange;
  DataRange yRange;

  QString toString() const override {
    QString result;
    result.reserve(points.size() * 70);

    for (size_t i = 0; i < points.size(); ++i) {
      result.append(QString("(%1, %2)").arg(points[i].x()).arg(points[i].y()));
      if (i < points.size() - 1) {
        result.append(", ");
      }
    }

    return QString("XYSeriesRenderData({\n    isValid = %1,\n    xRange = "
                   "%2,\n    yRange = %3,\n    points = [%4]\n})")
        .arg(valid ? "true" : "false")
        .arg(xRange.toString())
        .arg(yRange.toString())
        .arg(result);
  }
};

struct SeriesRenderPayload {
  int seriesIndex = -1;
  std::unique_ptr<RenderData> data;
};

struct AxisPayload {
  std::unique_ptr<RenderData> data;
};

struct PlotContext {
  QRectF itemRect;
  QRectF plotArea;

  DataRange xRange;
  DataRange yRange;

  AxisRange xAxisRange;
  AxisRange yAxisRange;

  ChartEnums::AxisPositions axisPositions;
};

struct ChartRenderPackage {
  std::vector<SeriesRenderPayload> seriesPayloads;
  AxisPayload xAxisPayload;
  AxisPayload yAxisPayload;
};

} // namespace ChartPlotter

#pragma once

#include "ChartPlotter/types/DataRange.hpp"

#include <QtQml>

namespace ChartPlotter {

class ViewportController : public QObject {
  Q_OBJECT
  QML_UNCREATABLE("ViewportController should be created in C++ side only")

public:
  explicit ViewportController(QObject *parent = nullptr);

  void setRange(DataRange range);
  void resetZoom();
  const DataRange &getVisibleRange() const;
  void zoom(QRectF viewport, QPointF mousePos, int steps);
  void pan(QRectF viewport, double deltaXPixels);
  void setTargetTickCount(int tickCount);

private:
  /**
   * Note that visible range can be bigger, smaller or the same as data range.
   * They are not neccessary need to be matched.
   *
   * E.g. Data range is 14 - 180, visible range can be 0 - 200 for nice view. If
   * we zoom the data, visible range can be 50 - 100.
   */
  DataRange m_dataRange;
  DataRange m_visibleDataRange;
  bool m_isAutoScaled = true;
  int m_targetTickCount = 6;
};

} // namespace ChartPlotter

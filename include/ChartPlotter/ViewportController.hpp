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

private:
  DataRange m_dataRange;
  DataRange m_visibleDataRange;
  bool m_isAutoScaled = true;

  DataRange expandToNiceBounds(const DataRange &rawRange,
                               int targetTickCount) const;
};

} // namespace ChartPlotter

#include "ChartPlotter/ViewportController.hpp"
#include "ChartPlotter/constants/ChartConstants.hpp"
#include "ChartPlotter/utils/RenderMath.hpp"

namespace ChartPlotter {

namespace {
inline const float zoomFactor = 0.9;
}

ViewportController::ViewportController(QObject *parent) : QObject(parent) {}

void ViewportController::setRange(DataRange newRange) {
  const double oldMax = m_dataRange.valid ? m_dataRange.max : newRange.max;
  m_dataRange = newRange;

  if (m_isAutoScaled || !m_visibleDataRange.valid) {
    m_visibleDataRange = expandToNiceBounds(m_dataRange, 6);
    return;
  }

  const double currentWindowWidth =
      m_visibleDataRange.max - m_visibleDataRange.min;

  // We are in a zoomed state. Evaluate user intent.
  if (m_visibleDataRange.max >= oldMax - ChartConstants::EPSILON) {
    // Live Edge Tracking (Trailing Window)
    m_visibleDataRange.max = m_dataRange.max;
    m_visibleDataRange.min = std::clamp(m_dataRange.max - currentWindowWidth,
                                        m_dataRange.min, m_dataRange.max);
  } else {
    // Historical Inspection
    m_visibleDataRange.min =
        std::clamp(m_visibleDataRange.min, m_dataRange.min, m_dataRange.max);
    m_visibleDataRange.max =
        std::clamp(m_visibleDataRange.min + currentWindowWidth, m_dataRange.min,
                   m_dataRange.max);
  }
}

void ViewportController::resetZoom() {
  m_isAutoScaled = true;
  m_visibleDataRange = expandToNiceBounds(m_dataRange, 6);
}

const DataRange &ViewportController::getVisibleRange() const {
  return m_visibleDataRange;
}

void ViewportController::zoom(QRectF viewport, QPointF mousePos, int steps) {
  if (!m_visibleDataRange.valid ||
      m_visibleDataRange.min >= m_visibleDataRange.max || steps == 0) {
    return;
  }

  m_isAutoScaled = false;

  const double factor = std::pow(0.9, steps);
  const double currentRange = m_visibleDataRange.max - m_visibleDataRange.min;
  const double newRange = currentRange * factor;
  const DataRange maxAllowedBounds = expandToNiceBounds(m_dataRange, 6);
  const double maxAllowedWidth = maxAllowedBounds.max - maxAllowedBounds.min;
  const bool isTrackingLiveEdge =
      (m_visibleDataRange.max >= m_dataRange.max - ChartConstants::EPSILON);

  double t;
  double anchor;
  if (isTrackingLiveEdge) {
    // We are at the live edge. Force the anchor to the right side.
    t = 1.0;
    anchor = m_visibleDataRange.max;
  } else {
    // We are in the past. Anchor to the mouse cursor.
    t = std::clamp(mousePos.x() / viewport.width(), 0.0, 1.0);
    // nicer UI
    if (t <= 0.3) {
      t = 0;
    }
    if (t >= 0.7) {
      t = 1;
    }

    anchor = m_visibleDataRange.min + t * currentRange;
  }

  double newMin = anchor - t * newRange;
  double newMax = anchor + (1.0 - t) * newRange;

  if (newMax - newMin > maxAllowedWidth) {
    // User zoomed out too far. Cap it to the padded limits and restore
    // auto-scale.
    newMin = maxAllowedBounds.min;
    newMax = maxAllowedBounds.max;
    m_isAutoScaled = true;
  } else {
    // Standard clamping to prevent panning into the void.
    newMin = std::clamp(newMin, maxAllowedBounds.min, maxAllowedBounds.max);
    newMax = std::clamp(newMax, maxAllowedBounds.min, maxAllowedBounds.max);
  }

  m_visibleDataRange.min = std::min(newMin, newMax);
  m_visibleDataRange.max = std::max(newMin, newMax);
}

void ViewportController::pan(QRectF viewport, double deltaXPixels) {
  if (!m_visibleDataRange.valid ||
      m_visibleDataRange.min >= m_visibleDataRange.max) {
    return;
  }

  if (m_isAutoScaled) {
    // full chart, no dragging
    return;
  }

  const double currentRange = m_visibleDataRange.max - m_visibleDataRange.min;
  const double dataDelta = (deltaXPixels / viewport.width()) * currentRange;
  const DataRange maxAllowedBounds = expandToNiceBounds(m_dataRange, 6);

  double newMin = m_visibleDataRange.min - dataDelta;
  double newMax = m_visibleDataRange.max - dataDelta;

  // Prevent the user from dragging the chart entirely off-screen into the void
  if (newMin < maxAllowedBounds.min) {
    newMin = maxAllowedBounds.min;
    newMax = newMin + currentRange;
  } else if (newMax > maxAllowedBounds.max) {
    newMax = maxAllowedBounds.max;
    newMin = newMax - currentRange;
  }

  m_visibleDataRange.min = newMin;
  m_visibleDataRange.max = newMax;
}

DataRange ViewportController::expandToNiceBounds(const DataRange &rawRange,
                                                 int targetTickCount) const {
  if (!rawRange.valid || rawRange.min >= rawRange.max) {
    return rawRange;
  }

  const double rawDiff = rawRange.max - rawRange.min;

  double niceRange = RenderMath::niceNumber(rawDiff, false);
  double step = RenderMath::niceNumber(niceRange / (targetTickCount - 1), true);

  DataRange paddedRange;
  paddedRange.min = std::floor(rawRange.min / step) * step;
  paddedRange.max = std::ceil(rawRange.max / step) * step;
  paddedRange.valid = true;

  return paddedRange;
}

} // namespace ChartPlotter

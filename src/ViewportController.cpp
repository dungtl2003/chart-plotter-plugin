#include "ChartPlotter/ViewportController.hpp"
#include "ChartPlotter/constants/ChartConstants.hpp"
#include "ChartPlotter/utils/RenderMath.hpp"

namespace ChartPlotter {

namespace {
inline const float zoomFactor = 0.9;
}

ViewportController::ViewportController(QObject *parent) : QObject(parent) {}

DataRange ViewportController::maxBounds() const {
  if (!m_useNiceBounds) {
    return m_dataRange;
  }
  return RenderMath::expandToNiceBounds(m_dataRange, m_targetTickCount).range;
}

void ViewportController::setUseNiceBounds(bool useNiceBounds) {
  m_useNiceBounds = useNiceBounds;
}

void ViewportController::setRange(DataRange newRange) {
  const double oldMax = m_dataRange.valid ? m_dataRange.max : newRange.max;
  m_dataRange = newRange;

  if (m_isAutoScaled || !m_visibleDataRange.valid) {
    m_visibleDataRange = maxBounds();
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
  m_visibleDataRange = maxBounds();
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
  double newRange = currentRange * factor;

  // Floor the window so the user cannot zoom in past the dataset's meaningful
  // resolution. Without this the range collapses toward zero, which makes the
  // axis emit many ticks with the same rounded label and drives the tick step
  // to zero (an unbounded loop that crashes). The floor is dataset-relative: a
  // fraction of the full span, but never below a fraction of the values'
  // magnitude (needed for large-magnitude axes like epoch-ms dates).
  const double dataSpan = m_dataRange.max - m_dataRange.min;
  const double magnitude =
      std::max(std::abs(m_dataRange.min), std::abs(m_dataRange.max));
  const double minVisibleWidth =
      std::max(dataSpan * ChartConstants::MIN_ZOOM_SPAN_FRACTION,
               magnitude * ChartConstants::MIN_ZOOM_MAGNITUDE_FRACTION);
  if (steps > 0 && minVisibleWidth > 0.0) {
    // steps > 0 is zoom-in (factor < 1, window shrinking).
    newRange = std::max(newRange, minVisibleWidth);
  }

  const DataRange maxAllowedBounds = maxBounds();
  const double maxAllowedWidth = maxAllowedBounds.max - maxAllowedBounds.min;
  const bool isTrackingLiveEdge =
      (m_visibleDataRange.max >= m_dataRange.max - ChartConstants::EPSILON);
  const double absoluteRange = m_dataRange.max - m_dataRange.min;

  double t;
  double anchor;
  t = std::clamp(mousePos.x() / viewport.width(), 0.0, 1.0);
  // nicer UI
  if (t <= 0.2) {
    t = 0;
  }
  if (t >= 0.8) {
    t = 1;
  }

  anchor = m_visibleDataRange.min + t * currentRange;

  double newMin = anchor - t * newRange;
  double newMax = anchor + (1.0 - t) * newRange;

  /**
   * 2 cases:
   * - User zoomed too far, cap the limit.
   * - Currently lower than data range, but after zoom bigger than data range,
   * this mean it should be auto scale again
   */
  if (newMax - newMin > maxAllowedWidth ||
      ((newMax - newMin) > absoluteRange && currentRange <= absoluteRange)) {
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
  const DataRange maxAllowedBounds = maxBounds();

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

void ViewportController::setTargetTickCount(int tickCount) {
  m_targetTickCount = tickCount;
}

} // namespace ChartPlotter

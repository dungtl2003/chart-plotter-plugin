#include "ChartPlotter/series/LineSeries.hpp"

namespace ChartPlotter {

LineSeries::LineSeries(QObject *parent) : XYSeries(parent) {}

ChartEnums::SeriesType LineSeries::type() {
  return ChartEnums::SeriesType::Line;
}

QColor LineSeries::strokeColor() const { return m_strokeColor; }

void LineSeries::setStrokeColor(const QColor &color) {
  if (m_strokeColor == color) {
    return;
  }

  m_strokeColor = color;
  emit strokeColorChanged();
}

float LineSeries::strokeWidth() const { return m_strokeWidth; }

void LineSeries::setStrokeWidth(float width) {
  if (m_strokeWidth == width) {
    return;
  }

  m_strokeWidth = width;
  emit strokeWidthChanged();
}

bool LineSeries::markerVisible() const { return m_markerVisible; }

void LineSeries::setMarkerVisible(bool visible) {
  if (m_markerVisible == visible) {
    return;
  }

  m_markerVisible = visible;
  emit markerVisibleChanged();
}

QColor LineSeries::markerColor() const { return m_markerColor; }

void LineSeries::setMarkerColor(const QColor &color) {
  if (m_markerColor == color) {
    return;
  }

  m_markerColor = color;
  emit markerColorChanged();
}

float LineSeries::markerRadius() const { return m_markerRadius; }

void LineSeries::setMarkerRadius(float radius) {
  if (m_markerRadius == radius) {
    return;
  }

  m_markerRadius = radius;
  emit markerRadiusChanged();
}

float LineSeries::antialias() const { return m_antialias; }

void LineSeries::setAntialias(float antialias) {
  if (m_antialias == antialias) {
    return;
  }

  m_antialias = antialias;
  emit antialiasChanged();
}

} // namespace ChartPlotter

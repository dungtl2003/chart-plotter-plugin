#include "ChartPlotter/series/LineSeries.hpp"

namespace ChartPlotter {

LineSeries::LineSeries(QObject *parent) : XYSeries(parent) {}

ChartEnums::SeriesType LineSeries::type() const {
  return ChartEnums::SeriesType::Line;
}

QColor LineSeries::legendColor() const { return strokeColor(); }

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

ChartEnums::StrokePattern LineSeries::strokePattern() const {
  return m_strokePattern;
}
void LineSeries::setStrokePattern(ChartEnums::StrokePattern newPattern) {
  if (m_strokePattern == newPattern) {
    return;
  }

  m_strokePattern = newPattern;
  emit strokePatternChanged();
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

float LineSeries::antialias() const { return m_antialias; }
void LineSeries::setAntialias(float antialias) {
  if (m_antialias == antialias) {
    return;
  }

  m_antialias = antialias;
  emit antialiasChanged();
}

bool LineSeries::useGlobalStrokeWidth() const { return m_useGlobalStrokeWidth; }
void LineSeries::setUseGlobalStrokeWidth(bool useGlobal) {
  if (m_useGlobalStrokeWidth == useGlobal) {
    return;
  }

  m_useGlobalStrokeWidth = useGlobal;
  emit useGlobalStrokeWidthChanged();
}

bool LineSeries::useGlobalAntialias() const { return m_useGlobalAntialias; }
void LineSeries::setUseGlobalAntialias(bool useGlobal) {
  if (m_useGlobalAntialias == useGlobal) {
    return;
  }

  m_useGlobalAntialias = useGlobal;
  emit useGlobalAntialiasChanged();
}

} // namespace ChartPlotter

#include "ChartPlotter/series/BarSeries.hpp"

namespace ChartPlotter {

BarSeries::BarSeries(QObject *parent) : XYSeries(parent) {}

ChartEnums::SeriesType BarSeries::type() const {
  return ChartEnums::SeriesType::Bar;
}

QColor BarSeries::legendColor() const { return m_color; }

QColor BarSeries::color() const { return m_color; }

void BarSeries::setColor(const QColor &color) {
  if (m_color == color) {
    return;
  }

  m_color = color;
  emit colorChanged();
}

} // namespace ChartPlotter

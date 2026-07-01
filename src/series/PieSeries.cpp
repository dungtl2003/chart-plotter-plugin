#include "ChartPlotter/series/PieSeries.hpp"

namespace ChartPlotter {

PieSeries::PieSeries(QObject *parent) : AbstractSeries(parent) {};

QString PieSeries::label() const { return m_label; }
void PieSeries::setLabel(const QString &newLabel) {
  if (m_label == newLabel) {
    return;
  }

  m_label = newLabel;
  emit labelChanged();
}

QString PieSeries::value() const { return m_value; }
void PieSeries::setValue(const QString &newValue) {
  if (m_value == newValue) {
    return;
  }

  m_value = newValue;
  emit valueChanged();
}

QVariantList PieSeries::colors() const { return m_colors; }
void PieSeries::setColors(const QVariantList &newColors) {
  if (m_colors == newColors) {
    return;
  }

  m_colors = newColors;
  emit colorsChanged();
}

ChartEnums::SeriesType PieSeries::type() const {
  return ChartEnums::SeriesType::Pie;
}

} // namespace ChartPlotter

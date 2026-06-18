#include "ChartPlotter/series/AbstractSeries.hpp"

namespace ChartPlotter {

AbstractSeries::AbstractSeries(QObject *parent) : QObject(parent) {}

QPointer<DataSource> AbstractSeries::source() const { return m_source; }
void AbstractSeries::setSource(QPointer<DataSource> newSource) {
  if (m_source == newSource) {
    return;
  }

  m_source = newSource;
  emit sourceChanged();
}

QString AbstractSeries::name() const { return m_name; }
void AbstractSeries::setName(const QString &name) {
  if (m_name == name) {
    return;
  }
  m_name = name;
  emit nameChanged();
}

QColor AbstractSeries::legendColor() const { return Qt::black; }

} // namespace ChartPlotter

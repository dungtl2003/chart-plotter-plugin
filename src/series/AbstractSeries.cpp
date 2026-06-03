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

} // namespace ChartPlotter

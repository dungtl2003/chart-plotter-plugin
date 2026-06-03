#include "ChartPlotter/series/XYSeries.hpp"

namespace ChartPlotter {

XYSeries::XYSeries(QObject *parent) : AbstractSeries(parent) {};

QString XYSeries::x() const { return m_x; }
void XYSeries::setX(const QString &newX) {
  if (m_x == newX) {
    return;
  }

  m_x = newX;
  emit xChanged();
}

QString XYSeries::y() const { return m_y; }
void XYSeries::setY(const QString &newY) {
  if (m_y == newY) {
    return;
  }

  m_y = newY;
  emit yChanged();
}

int XYSeries::xColumn() const { return m_xColumn; };
void XYSeries::setXColumn(int newXColumn) {
  if (m_xColumn == newXColumn) {
    return;
  }

  m_xColumn = newXColumn;
  emit xColumnChanged();
}

int XYSeries::yColumn() const { return m_yColumn; };
void XYSeries::setYColumn(int newYColumn) {
  if (m_yColumn == newYColumn) {
    return;
  }

  m_yColumn = newYColumn;
  emit yColumnChanged();
}

ColumnBinding XYSeries::xBinding() const {
  if (m_xColumn >= 0) {
    return ColumnBinding::byIndex(m_xColumn);
  }

  return ColumnBinding::byName(m_x);
}

ColumnBinding XYSeries::yBinding() const {
  if (m_yColumn >= 0) {
    return ColumnBinding::byIndex(m_yColumn);
  }

  return ColumnBinding::byName(m_y);
}

} // namespace ChartPlotter

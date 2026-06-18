#include "ChartPlotter/legend/LegendModel.hpp"

namespace ChartPlotter {

LegendModel::LegendModel(QObject *parent) : QAbstractListModel(parent) {}

int LegendModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : m_entries.size();
}

QVariant LegendModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
    return {};
  }
  const LegendEntry &e = m_entries.at(index.row());
  switch (role) {
  case NameRole:
    return e.name;
  case ColorRole:
    return e.color;
  case VisibleRole:
    return e.visible;
  default:
    return {};
  }
}

QHash<int, QByteArray> LegendModel::roleNames() const {
  return {
      {NameRole, "name"},
      {ColorRole, "color"},
      {VisibleRole, "visible"},
  };
}

void LegendModel::setEntries(const QVector<LegendEntry> &entries) {
  beginResetModel();
  m_entries = entries;
  endResetModel();
}

bool LegendModel::isVisible(int row) const {
  if (row < 0 || row >= m_entries.size()) {
    return true;
  }
  return m_entries.at(row).visible;
}

void LegendModel::setSeriesVisible(int row, bool visible) {
  if (row < 0 || row >= m_entries.size() || m_entries[row].visible == visible) {
    return;
  }
  m_entries[row].visible = visible;
  const QModelIndex idx = index(row);
  emit dataChanged(idx, idx, {VisibleRole});
  emit visibilityChanged(row, visible);
}

void LegendModel::toggleSeries(int row) {
  if (row < 0 || row >= m_entries.size()) {
    return;
  }
  setSeriesVisible(row, !m_entries.at(row).visible);
}

} // namespace ChartPlotter

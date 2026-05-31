#include "ChartPlotter/data/Column.hpp"

namespace ChartPlotter {

Column::Column(QObject *parent) : QObject(parent) {}

int Column::idx() const { return m_idx; }
void Column::setIdx(int newIdx) {
  if (m_idx == newIdx) {
    return;
  }

  m_idx = newIdx;
  emit idxChanged();
}

QString Column::name() const { return m_name; }

void Column::setName(const QString &newName) {
  if (m_name == newName) {
    return;
  }

  m_name = newName;
  emit nameChanged();
}

ChartEnums::DataType Column::type() const { return m_type; }

void Column::setType(ChartEnums::DataType newType) {
  if (m_type == newType) {
    return;
  }

  m_type = newType;
  emit typeChanged();
}

} // namespace ChartPlotter

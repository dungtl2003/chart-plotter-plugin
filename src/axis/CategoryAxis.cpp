#include "ChartPlotter/axis/CategoryAxis.hpp"

namespace ChartPlotter {

int CategoryAxis::intern(const QString &label) {
  auto it = m_indexByLabel.constFind(label);
  if (it != m_indexByLabel.constEnd()) {
    return it.value();
  }
  const int index = static_cast<int>(m_labels.size());
  m_labels.push_back(label);
  m_indexByLabel.insert(label, index);
  return index;
}

int CategoryAxis::indexOf(const QString &label) const {
  return m_indexByLabel.value(label, -1);
}

bool CategoryAxis::isEmpty() const { return m_labels.isEmpty(); }
int CategoryAxis::size() const { return static_cast<int>(m_labels.size()); }
const QString &CategoryAxis::labelAt(int i) const { return m_labels.at(i); }

} // namespace ChartPlotter

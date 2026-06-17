#pragma once

#include <QHash>
#include <QString>
#include <QVector>

namespace ChartPlotter {

// Maps category labels to stable integer positions on one axis.
// Built once per axis, then shared read-only across every series that
// uses that axis, so a given label always lands on the same position.
class CategoryAxis {
public:
  // Adds the label if new; returns its position. Used during the build pass.
  int intern(const QString &label);

  // Position of an already-interned label, or -1. Used by strategies.
  int indexOf(const QString &label) const;

  bool isEmpty() const;
  int size() const;
  const QString &labelAt(int i) const;

private:
  QVector<QString> m_labels;
  QHash<QString, int> m_indexByLabel;
};

} // namespace ChartPlotter

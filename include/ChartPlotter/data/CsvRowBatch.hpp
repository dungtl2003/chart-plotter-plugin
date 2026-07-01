#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QVector>

#include <string_view>

namespace ChartPlotter {

// A batch of fully-parsed CSV records expressed as byte spans, with no
// per-cell QString/QVariant allocations. Consumers convert spans straight to
// double via std::from_chars — the whole point of the fast path.
struct CsvRowBatch {
  QByteArray storage;
  QVector<quint32> fieldStart;
  QVector<quint32> fieldLen;
  QVector<quint32> rowStart; // size = rowCount + 1 when non-empty

  qsizetype rowCount() const {
    return rowStart.isEmpty() ? 0 : rowStart.size() - 1;
  }

  std::string_view field(quint32 fieldIndex) const {
    return std::string_view(storage.constData() + fieldStart[fieldIndex],
                            fieldLen[fieldIndex]);
  }
};

} // namespace ChartPlotter

Q_DECLARE_METATYPE(ChartPlotter::CsvRowBatch)

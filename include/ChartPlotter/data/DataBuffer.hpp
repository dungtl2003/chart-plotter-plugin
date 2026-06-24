#pragma once

#include "ChartPlotter/data/DataRow.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector>

namespace ChartPlotter {

struct DataChunk {
  static constexpr int CHUNK_SIZE = 10000;
  QVector<QVariant> values;

  DataChunk();
};

struct ColumnSnapshot {
  QString name;
  ChartEnums::DataType type;

  std::vector<std::shared_ptr<const DataChunk>> chunks;
};

struct DataSnapshot {
  QVector<ColumnSnapshot> columns;
  QHash<QString, quint64> columnIndex;

  quint64 rowCount = 0;
  quint64 columnCount = 0;
  quint64 version = 0;

  QString toString() const;
  ChartEnums::DataType columnType(int idx) const;
  QString columnName(int idx) const;
  QVariant valueAt(quint64 col, quint64 row) const;
};

struct MutableDataColumn {
  QString name;
  ChartEnums::DataType type;
  std::vector<std::shared_ptr<DataChunk>> chunks;

  void appendValue(const QVariant &value);
  quint64 size() const;
  QString toString() const;
};

struct ColumnInitField {
  QString name;
  ChartEnums::DataType ty = ChartEnums::DataType::Unknown;
};

class DataBuffer {

public:
  void clear();

  void initColumns(const QVector<ColumnInitField> &columnInitFields);
  void appendRow(const DataRow &row);
  void appendRows(const QVector<DataRow> &rows);

  quint64 rowCount() const;
  quint64 columnCount() const;

  bool hasColumn(const QString &name) const;
  std::optional<std::reference_wrapper<MutableDataColumn>>
  column(const QString &name);
  std::optional<std::reference_wrapper<MutableDataColumn>> column(qint64 idx);

  QVariant valueAt(const QString &columnName, int row);

  QVector<QString> columnNames() const;
  QString toString() const;

  void setIsValid(bool newIsValid);
  bool isValid() const;

  DataSnapshot snapshot();

private:
  QVector<MutableDataColumn> m_columns;
  QHash<QString, quint64> m_columnIndex;
  quint64 m_rowCount = 0;
  bool m_isValid = true;
  quint64 m_version = 0;

  std::mutex m_snapshotMutex;

  void rebuildColumnIndex();
  void normalizeColumnSizes();
};

} // namespace ChartPlotter

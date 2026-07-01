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
  QVector<double> values;

  DataChunk();
};

struct ColumnSnapshot {
  QString name;
  ChartEnums::DataType type;

  std::vector<std::shared_ptr<const DataChunk>> chunks;
  QVector<QString> categories;
};

struct DataSnapshot {
  QVector<ColumnSnapshot> columns;
  QHash<QString, qint64> columnIndex;

  quint64 epochId = 0;
  qint64 rowCount = 0;
  qint64 columnCount = 0;
  qint64 version = 0;
  // Absolute id of the first live row (= number of rows evicted from the front
  // by the max-rows cap). Lets consumers keep stable, append-only row ids even
  // though the physical window slides. `valueAt(row)` indexes the LIVE window
  // [0, rowCount); add `firstRowId` to get the absolute id.
  qint64 firstRowId = 0;

  QString toString() const;
  ChartEnums::DataType columnType(qint64 idx) const;
  QString columnName(qint64 idx) const;
  double valueAt(qint64 col, qint64 row) const;
  QString categoryName(qint64 col, double val) const;
};

struct MutableDataColumn {
  QString name;
  ChartEnums::DataType type;
  std::vector<std::shared_ptr<DataChunk>> chunks;

  QHash<QString, double> stringToId;
  QVector<QString> idToString;

  // Cached date-parse strategy for this column, decided from the first value:
  // 0 = undetermined, 1 = fast hand-rolled UTC-ISO parser, 2 = QDateTime ISO
  // parse, 3 = generic QVariant parse (slow backstop). Only meaningful when
  // type == Date.
  int dateParseMode = 0;

  void appendValue(double value);
  // Bulk append `n` values, filling chunks with memcpy-sized runs instead of
  // one push_back per value. Hot path for file ingest.
  void appendValues(const double *data, qint64 n);
  qint64 size() const;
  QString toString() const;
};

struct ColumnInitField {
  QString name;
  ChartEnums::DataType ty = ChartEnums::DataType::Unknown;
};

class DataBuffer {

public:
  DataBuffer();

  void clear();

  void initColumns(const QVector<ColumnInitField> &columnInitFields);

  void appendRow(const QVector<double> &rowValues);
  void appendRows(const QVector<QVector<double>> &rows);
  // Column-major bulk append: cols[c] holds `n` values for column c. Locks once
  // and appends each column in bulk. Hot path for file ingest.
  void appendColumnsBulk(const QVector<QVector<double>> &cols, qint64 n);

  qint64 rowCount() const;
  qint64 columnCount() const;

  // Cap the number of rows kept in memory. When exceeded, whole front chunks are
  // evicted (oldest rows dropped permanently). <= 0 means unlimited. Safe to call
  // from any thread; trims immediately.
  void setMaxRows(qint64 maxRows);

  bool hasColumn(const QString &name) const;
  std::optional<std::reference_wrapper<MutableDataColumn>>
  column(const QString &name);
  std::optional<std::reference_wrapper<MutableDataColumn>> column(qint64 idx);

  double valueAt(const QString &columnName, qint64 row);

  QVector<QString> columnNames() const;
  QString toString() const;

  void setIsValid(bool newIsValid);
  bool isValid() const;

  DataSnapshot snapshot();

private:
  QVector<MutableDataColumn> m_columns;
  QHash<QString, qint64> m_columnIndex;

  quint64 m_epochId = 0;
  qint64 m_rowCount = 0; // live rows currently in memory
  qint64 m_firstRowId = 0; // absolute id of live row 0 (rows evicted so far)
  qint64 m_maxRows = -1;   // row cap; <= 0 means unlimited
  bool m_isValid = true;
  qint64 m_version = 0;

  std::mutex m_snapshotMutex;

  void rebuildColumnIndex();
  void normalizeColumnSizes();
  // Evict whole front chunks while over the cap. Caller must hold the mutex.
  void trimToCapLocked();
};

} // namespace ChartPlotter

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

  void appendValue(double value);
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

  qint64 rowCount() const;
  qint64 columnCount() const;

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
  qint64 m_rowCount = 0;
  bool m_isValid = true;
  qint64 m_version = 0;

  std::mutex m_snapshotMutex;

  void rebuildColumnIndex();
  void normalizeColumnSizes();
};

} // namespace ChartPlotter

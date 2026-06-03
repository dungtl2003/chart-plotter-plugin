#pragma once

#include "ChartPlotter/data/DataColumn.hpp"
#include "ChartPlotter/data/DataRow.hpp"

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector>

namespace ChartPlotter {

struct DataSnapshot {
  QVector<DataColumn> columns;
  QHash<QString, int> columnIndex;
  int rowCount = 0;
  int columnCount = 0;
  quint64 version = 0;

  QString toString() const;
  ChartEnums::DataType columnType(int idx) const;
  QString columnName(int idx) const;
  QVariant valueAt(int col, int row) const;
};

struct ColumnInitField {
  QString name;
  ChartEnums::DataType ty = ChartEnums::DataType::Unknown;
};

class DataBuffer {

public:
  void clear();

  void initColumns(const QVector<ColumnInitField> columnInitFields);
  void appendRow(const DataRow &row);
  void appendRows(const QVector<DataRow> &rows);

  int rowCount() const;
  int columnCount() const;

  bool hasColumn(const QString &name) const;
  std::optional<std::reference_wrapper<DataColumn>> column(const QString &name);
  std::optional<std::reference_wrapper<DataColumn>> column(qint64 idx);

  QVariant valueAt(const QString &columnName, int row);

  QVector<QString> columnNames() const;
  QVector<QVariant> columnValues(const QString &columnName);
  QString toString() const;

  void setIsValid(bool newIsValid);
  bool isValid() const;

  DataSnapshot snapshot();

private:
  QVector<DataColumn> m_columns;
  QHash<QString, int> m_columnIndex;
  int m_rowCount = 0;
  bool m_isValid = true;
  quint64 m_version = 0;

  void rebuildColumnIndex();
  void normalizeColumnSizes();
};

} // namespace ChartPlotter

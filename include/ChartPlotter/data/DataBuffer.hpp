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

struct ColumnInitField {
  QString name;
  ChartEnums::DataType ty = ChartEnums::DataType::Unknown;
};

class DataBuffer : public QObject {
  Q_OBJECT

public:
  explicit DataBuffer(QObject *parent = nullptr);

  void clear();

  void setColumns(const QVector<DataColumn> &columns);
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

signals:
  void dataReset();
  void rowsInserted(int first, int last);
  void dataChanged();

private:
  QVector<DataColumn> m_columns;
  QHash<QString, int> m_columnIndex;
  int m_rowCount = 0;
  bool m_isValid = true;

  void rebuildColumnIndex();
  void normalizeColumnSizes();
};

} // namespace ChartPlotter

#include "ChartPlotter/data/DataBuffer.hpp"

namespace ChartPlotter {

DataBuffer::DataBuffer(QObject *parent) : QObject(parent) {}

void DataBuffer::clear() {
  m_columns.clear();
  m_columnIndex.clear();
  m_rowCount = 0;
}

void DataBuffer::setColumns(const QVector<DataColumn> &columns) {
  clear();

  m_columns = columns;

  /**
   * Technically, all columns should have the same row count, so we will take
   * rows from the first one.
   */
  // if (!columns.empty()) {
  //   m_rowCount = columns.at(0).values.size();
  // }

  for (qsizetype i = 0; i < m_columns.size(); ++i) {
    m_columnIndex.insert(m_columns.at(i).name, i);
  }
}

void DataBuffer::initColumns(const QVector<ColumnInitField> columnInitFields) {
  m_columns.clear();
  m_columnIndex.clear();
  m_columns.reserve(columnInitFields.size());
  m_columnIndex.reserve(columnInitFields.size());

  for (qsizetype i = 0; i < columnInitFields.size(); ++i) {
    m_columns.push_back(DataColumn{.name = columnInitFields.at(i).name,
                                   .type = columnInitFields.at(i).ty});
    m_columnIndex.insert(columnInitFields.at(i).name, i);
  }
}

void DataBuffer::appendRow(const DataRow &row) {
  if (m_columns.isEmpty()) {
    return;
  }

  const int insertedRow = m_rowCount;

  for (int column = 0; column < m_columns.size(); ++column) {
    if (column < row.values.size()) {
      m_columns[column].values.push_back(row.values.at(column));
    } else {
      m_columns[column].values.push_back(QVariant());
    }
  }

  ++m_rowCount;

  emit rowsInserted(insertedRow, insertedRow);
  emit dataChanged();
}

void DataBuffer::appendRows(const QVector<DataRow> &rows) {
  if (m_columns.isEmpty() || rows.isEmpty()) {
    return;
  }

  const int firstInserted = m_rowCount;

  for (const DataRow &row : rows) {
    for (int column = 0; column < m_columns.size(); ++column) {
      if (column < row.values.size()) {
        m_columns[column].values.push_back(row.values.at(column));
      } else {
        m_columns[column].values.push_back(QVariant());
      }
    }

    ++m_rowCount;
  }

  const int lastInserted = m_rowCount - 1;

  emit rowsInserted(firstInserted, lastInserted);
  emit dataChanged();
}

int DataBuffer::rowCount() const { return m_rowCount; }
int DataBuffer::columnCount() const { return m_columns.size(); }

bool DataBuffer::hasColumn(const QString &name) const {
  return m_columnIndex.contains(name);
}

std::optional<std::reference_wrapper<DataColumn>>
DataBuffer::column(const QString &name) {
  try {
    return m_columns[m_columnIndex.value(name)];
  } catch (std::exception e) {
    return std::nullopt;
  }
}

std::optional<std::reference_wrapper<DataColumn>>
DataBuffer::column(qint64 idx) {
  try {
    return m_columns[idx];
  } catch (std::exception e) {
    return std::nullopt;
  }
}

QVariant DataBuffer::valueAt(const QString &columnName, int row) {
  try {
    return column(columnName)->get().values.at(row);
  } catch (std::exception e) {
    return QVariant();
  }
}

QVector<QString> DataBuffer::columnNames() const {
  QVector<QString> names;
  names.reserve(m_columns.size());

  for (const DataColumn &column : m_columns) {
    names.push_back(column.name);
  }

  return names;
}

QVector<QVariant> DataBuffer::columnValues(const QString &columnName) {
  auto col = column(columnName);
  if (!col.has_value()) {
    return QVector<QVariant>();
  }

  return col->get().values;
}

QString DataBuffer::toString() const {
  if (!isValid()) {
    return "DataBuffer(invalid)";
  }

  QString result;
  result.reserve(m_columns.size() * 33);

  for (size_t i = 0; i < m_columns.size(); ++i) {
    result.append("    " + m_columns[i].toString());
    if (i < m_columns.size() - 1) {
      result.append(",\n");
    }
  }

  return "DataBuffer(\n" + result + "\n)";
}

void DataBuffer::setIsValid(bool newIsValid) { m_isValid = newIsValid; }
bool DataBuffer::isValid() const { return m_isValid; }

void DataBuffer::rebuildColumnIndex() {
  m_columnIndex.clear();

  for (int i = 0; i < m_columns.size(); ++i) {
    const QString &name = m_columns.at(i).name;

    if (name.isEmpty()) {
      continue;
    }

    if (m_columnIndex.contains(name)) {
      continue;
    }

    m_columnIndex.insert(name, i);
  }
}

void DataBuffer::normalizeColumnSizes() {
  int maxSize = 0;

  for (const DataColumn &column : m_columns) {
    maxSize = std::max(maxSize, int(column.values.size()));
  }

  for (DataColumn &column : m_columns) {
    while (column.values.size() < maxSize) {
      column.values.push_back(QVariant());
    }
  }

  m_rowCount = maxSize;
}

} // namespace ChartPlotter

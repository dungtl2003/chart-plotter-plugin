#include "ChartPlotter/data/DataBuffer.hpp"

namespace ChartPlotter {

DataChunk::DataChunk() { values.reserve(CHUNK_SIZE); }

QString DataSnapshot::toString() const {
  QString result;
  result.reserve(columns.size() * 33);

  for (qint64 i = 0; i < columns.size(); ++i) {
    result.append(QString("    ColumnSnapshot(%1, chunks=%2)")
                      .arg(columns[i].name)
                      .arg(columns[i].chunks.size()));
    if (i < columns.size() - 1) {
      result.append(",\n");
    }
  }

  return QString(
             "DataSnapshot({version = %1, rowCount = %2, columns = [\n%3\n]})")
      .arg(version)
      .arg(rowCount)
      .arg(result);
}

ChartEnums::DataType DataSnapshot::columnType(qint64 idx) const {
  try {
    return columns.at(idx).type;
  } catch (std::exception e) {
    return ChartEnums::DataType::Unknown;
  }
}

QString DataSnapshot::columnName(qint64 idx) const {
  try {
    return columns.at(idx).name;
  } catch (std::exception e) {
    return QString();
  }
}

double DataSnapshot::valueAt(qint64 col, qint64 row) const {
  if (col < 0 || col >= columns.size() || row < 0 || row >= rowCount) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  qint64 chunkIdx = row / DataChunk::CHUNK_SIZE;
  qint64 localIdx = row % DataChunk::CHUNK_SIZE;

  return columns[col].chunks[chunkIdx]->values[localIdx];
}

void MutableDataColumn::appendValue(double value) {
  if (chunks.empty() || chunks.back()->values.size() == DataChunk::CHUNK_SIZE) {
    chunks.push_back(std::make_shared<DataChunk>());
  }
  chunks.back()->values.push_back(value);
}

qint64 MutableDataColumn::size() const {
  if (chunks.empty()) {
    return 0;
  }
  return (chunks.size() - 1) * DataChunk::CHUNK_SIZE +
         chunks.back()->values.size();
}

QString MutableDataColumn::toString() const {
  return QString("Column(%1, type=%2, chunks=%3, size=%4)")
      .arg(name)
      .arg(static_cast<int>(type))
      .arg(chunks.size())
      .arg(size());
}

void DataBuffer::clear() {
  std::lock_guard<std::mutex> locker(m_snapshotMutex);
  m_columns.clear();
  m_columnIndex.clear();
  m_rowCount = 0;
}

void DataBuffer::initColumns(const QVector<ColumnInitField> &columnInitFields) {
  std::lock_guard<std::mutex> locker(m_snapshotMutex);
  m_columns.clear();
  m_columnIndex.clear();
  m_columns.reserve(columnInitFields.size());
  m_columnIndex.reserve(columnInitFields.size());

  for (qsizetype i = 0; i < columnInitFields.size(); ++i) {
    MutableDataColumn col;
    col.name = columnInitFields.at(i).name;
    col.type = columnInitFields.at(i).ty;
    m_columns.push_back(std::move(col));
    m_columnIndex.insert(columnInitFields.at(i).name, i);
  }
}

void DataBuffer::appendRow(const QVector<double> &rowValues) {
  if (m_columns.isEmpty())
    return;
  for (qint64 column = 0; column < m_columns.size(); ++column) {
    if (column < rowValues.size()) {
      m_columns[column].appendValue(rowValues.at(column));
    } else {
      m_columns[column].appendValue(std::numeric_limits<double>::quiet_NaN());
    }
  }
  ++m_rowCount;
}

void DataBuffer::appendRows(const QVector<QVector<double>> &rows) {
  if (m_columns.isEmpty() || rows.isEmpty())
    return;
  for (const auto &row : rows) {
    appendRow(row);
  }
}

qint64 DataBuffer::rowCount() const { return m_rowCount; }
qint64 DataBuffer::columnCount() const { return m_columns.size(); }

bool DataBuffer::hasColumn(const QString &name) const {
  return m_columnIndex.contains(name);
}

std::optional<std::reference_wrapper<MutableDataColumn>>
DataBuffer::column(const QString &name) {
  if (!m_columnIndex.contains(name)) {
    return std::nullopt;
  }

  return m_columns[m_columnIndex.value(name)];
}

std::optional<std::reference_wrapper<MutableDataColumn>>
DataBuffer::column(qint64 idx) {
  if (idx < 0 || idx >= m_columns.size()) {
    return std::nullopt;
  }

  return m_columns[idx];
}

double DataBuffer::valueAt(const QString &columnName, qint64 row) {
  auto col = column(columnName);
  if (!col.has_value() || row < 0 || row >= m_rowCount) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  qint64 chunkIdx = row / DataChunk::CHUNK_SIZE;
  qint64 localIdx = row % DataChunk::CHUNK_SIZE;
  return col->get().chunks[chunkIdx]->values[localIdx];
}

QString DataSnapshot::categoryName(qint64 col, double val) const {
  if (col >= static_cast<qint64>(columns.size()) || std::isnan(val)) {
    return QString();
  }
  qint64 idx = static_cast<int>(val);
  const auto &cats = columns[col].categories;
  if (idx < 0 || idx >= cats.size()) {
    return QString();
  }
  return cats[idx];
}

QVector<QString> DataBuffer::columnNames() const {
  QVector<QString> names;
  names.reserve(m_columns.size());

  for (const MutableDataColumn &column : m_columns) {
    names.push_back(column.name);
  }

  return names;
}

QString DataBuffer::toString() const {
  if (!isValid()) {
    return "DataBuffer(invalid)";
  }

  QString result;
  result.reserve(m_columns.size() * 33);

  for (qint64 i = 0; i < m_columns.size(); ++i) {
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

  for (qint64 i = 0; i < m_columns.size(); ++i) {
    const QString &name = m_columns.at(i).name;
    if (name.isEmpty() || m_columnIndex.contains(name)) {
      continue;
    }
    m_columnIndex.insert(name, i);
  }
}

void DataBuffer::normalizeColumnSizes() {
  qint64 maxSize = 0;
  for (const MutableDataColumn &column : m_columns) {
    maxSize = std::max(maxSize, column.size());
  }
  for (MutableDataColumn &column : m_columns) {
    while (column.size() < maxSize) {
      column.appendValue(std::numeric_limits<double>::quiet_NaN());
    }
  }
  m_rowCount = maxSize;
}

DataSnapshot DataBuffer::snapshot() {
  std::lock_guard<std::mutex> locker(m_snapshotMutex);

  DataSnapshot snap;
  snap.rowCount = m_rowCount;
  snap.columnCount = m_columns.size();
  snap.columnIndex = m_columnIndex;
  snap.version = ++m_version;

  snap.columns.reserve(m_columns.size());

  for (const auto &col : m_columns) {
    ColumnSnapshot colSnap;
    colSnap.name = col.name;
    colSnap.type = col.type;

    colSnap.chunks.assign(col.chunks.begin(), col.chunks.end());

    colSnap.categories = col.idToString;
    snap.columns.append(std::move(colSnap));
  }

  return snap;
}

} // namespace ChartPlotter

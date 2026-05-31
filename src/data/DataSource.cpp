#include "ChartPlotter/data/DataSource.hpp"

namespace ChartPlotter {

DataSource::DataSource(QObject *parent) : QObject(parent) {}

qint64 DataSource::totalColumns() const { return m_totalColumns; }
void DataSource::setTotalColumns(qint64 newTotalColumns) {
  if (m_totalColumns == newTotalColumns) {
    return;
  }

  m_totalColumns = newTotalColumns;
  emit totalColumnsChanged();
}

QUrl DataSource::url() const { return m_url; }
void DataSource::setUrl(const QUrl &newUrl) {
  if (m_url == newUrl) {
    return;
  }

  m_url = newUrl;
  emit urlChanged();
}

ChartEnums::DataFormat DataSource::format() const { return m_format; }
void DataSource::setFormat(ChartEnums::DataFormat newFormat) {
  if (m_format == newFormat) {
    return;
  }

  m_format = newFormat;
  emit formatChanged();
}

qint64 DataSource::chunkSize() const { return m_chunkSize; }
void DataSource::setChunkSize(qint64 newChunkSize) {
  if (m_chunkSize == newChunkSize) {
    return;
  }

  m_chunkSize = newChunkSize;
  emit chunkSizeChanged();
}

bool DataSource::hasHeader() const { return m_hasHeader; }
void DataSource::setHasHeader(bool newHasHeader) {
  if (m_hasHeader == newHasHeader) {
    return;
  }

  m_hasHeader = newHasHeader;
  emit hasHeaderChanged();
}

int DataSource::skipRows() const { return m_skipRows; }
void DataSource::setSkipRows(int newSkipRows) {
  if (m_skipRows == newSkipRows) {
    return;
  }

  m_skipRows = newSkipRows;
  emit skipRowsChanged();
}

QQmlListProperty<Column> DataSource::columns() {
  return QQmlListProperty<Column>(
      this, this, &DataSource::appendColumn, &DataSource::columnCount,
      &DataSource::columnAt, &DataSource::clearColumns);
}

const QVector<Column *> &DataSource::columnList() const { return m_columns; }

void DataSource::appendColumn(QQmlListProperty<Column> *list, Column *column) {
  auto *source = qobject_cast<DataSource *>(list->object);

  if (!source || !column) {
    return;
  }

  column->setParent(source);
  source->m_columns.append(column);

  emit source->columnsChanged();
}

qsizetype DataSource::columnCount(QQmlListProperty<Column> *list) {
  auto *source = qobject_cast<DataSource *>(list->object);

  if (!source) {
    return 0;
  }

  return source->m_columns.size();
}

Column *DataSource::columnAt(QQmlListProperty<Column> *list, qsizetype index) {
  auto *source = qobject_cast<DataSource *>(list->object);

  if (!source) {
    return nullptr;
  }

  if (index < 0 || index >= source->m_columns.size()) {
    return nullptr;
  }

  return source->m_columns.at(index);
}

void DataSource::clearColumns(QQmlListProperty<Column> *list) {
  auto *source = qobject_cast<DataSource *>(list->object);

  if (!source) {
    return;
  }

  qDeleteAll(source->m_columns);
  source->m_columns.clear();

  emit source->columnsChanged();
}

DataReadConfig DataSource::exportConfig() const {
  DataReadConfig config;

  config.totalColumns = m_totalColumns;
  config.url = m_url;
  config.format = m_format;
  config.chunkSize = m_chunkSize;
  config.hasHeader = m_hasHeader;
  config.skipRows = m_skipRows;

  for (const Column *column : m_columns) {
    if (!column) {
      continue;
    }

    config.columns.append(
        {.idx = column->idx(), .name = column->name(), .type = column->type()});
  }

  return config;
}

} // namespace ChartPlotter

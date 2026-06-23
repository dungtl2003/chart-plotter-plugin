#include "ChartPlotter/data/DataManager.hpp"
#include "ChartPlotter/data/parser/DataParserFactory.hpp"
#include "ChartPlotter/data/reader/DataReaderFactory.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"

namespace ChartPlotter {

namespace {

QByteArray eofChunk() {
  QByteArray chunk;
  chunk.append(static_cast<char>(EOF));
  return chunk;
}

} // namespace

DataManager::DataManager(QObject *parent) : QObject(parent) {}

void DataManager::setDataReadConfig(const DataReadConfig &config) {
  m_dataReadConfig = config;
}

void DataManager::setSourceId(int id) { m_sourceId = id; }
int DataManager::getSourceId() const { return m_sourceId; }

void DataManager::start() {
  const QUrl url = m_dataReadConfig.url;
  const ChartEnums::DataFormat format = m_dataReadConfig.format;

  m_buffer = std::make_shared<DataBuffer>();
  m_reader = DataReaderFactory::create(url, this);
  m_parser = DataParserFactory::create(format, this);
  m_bufferUpdater = new DataBufferUpdater(m_buffer, this);

  if (!m_reader || !m_parser) {
    emit errorOccurred("DataManager::start: unsupported source or format");
    return;
  }

  connect(this, &DataManager::started, m_reader, &AbstractDataReader::start,
          Qt::DirectConnection);
  connect(this, &DataManager::stopped, m_reader, &AbstractDataReader::stop,
          Qt::DirectConnection);
  connect(m_reader, &AbstractDataReader::dataReceived, this,
          &DataManager::onChunkReceived, Qt::DirectConnection);
  connect(m_reader, &AbstractDataReader::errorOccurred, this,
          &DataManager::onErrorOccurred, Qt::DirectConnection);

  connect(this, &DataManager::parseRequested, m_parser,
          &AbstractDataParser::parse, Qt::DirectConnection);
  connect(m_parser, &AbstractDataParser::rowsParsed, this,
          &DataManager::onRowsParsed, Qt::DirectConnection);

  connect(m_bufferUpdater, &DataBufferUpdater::errorOccurred, this,
          &DataManager::onErrorOccurred, Qt::DirectConnection);
  connect(m_bufferUpdater, &DataBufferUpdater::bufferUpdated, this,
          &DataManager::onBufferUpdated, Qt::DirectConnection);

  m_reader->setConfig(DataReaderConfig{
      .url = m_dataReadConfig.url,
      .chunkSize = m_dataReadConfig.chunkSize,
  });

  m_bufferUpdater->setConfig(DataBufferUpdaterConfig{
      .hasHeader = m_dataReadConfig.hasHeader,
      .skipRows = m_dataReadConfig.skipRows,
      .mode = m_reader->mode(),
      .columns = m_dataReadConfig.columns,
      .totalColumns = m_dataReadConfig.totalColumns,
  });

  emit started();
}

void DataManager::stop() { emit stopped(); }

void DataManager::onErrorOccurred(const QString &message) {
  emit errorOccurred(message);
}

void DataManager::onChunkReceived(const QByteArray &chunk) {
  if (!m_parser) {
    return;
  }

  if (chunk.isEmpty()) {
    return;
  }

  emit parseRequested(chunk);
}

void DataManager::onRowsParsed(const QVector<DataRow> &rows) {
  if (!m_bufferUpdater) {
    return;
  }

  if (rows.empty()) {
    return;
  }

  m_bufferUpdater->parseRows(rows);
}

void DataManager::onBufferUpdated() {
  if (!m_buffer) {
    return;
  }

  QMutexLocker locker(&m_snapshotMutex);
  m_currentSnapshot = std::move(m_buffer->snapshot());

  // CP_DEBUG("buffer updated");
  // m_bufferUpdated = true;
  // emit snapshotReady(m_buffer->snapshot());
}

const DataSnapshot &DataManager::getLatestSnapshot() {
  QMutexLocker locker(&m_snapshotMutex);
  return m_currentSnapshot;
}

} // namespace ChartPlotter

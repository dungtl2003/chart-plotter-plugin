#include "ChartPlotter/data/DataManager.hpp"
#include "ChartPlotter/data/parser/DataParserFactory.hpp"
#include "ChartPlotter/data/parser/FastCsvDataParser.hpp"
#include "ChartPlotter/data/reader/DataReaderFactory.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

namespace ChartPlotter {

namespace {

QByteArray eofChunk() {
  QByteArray chunk;
  chunk.append(static_cast<char>(EOF));
  return chunk;
}

// In-flight chunk budget for the reader thread. With ~2 MB chunks this caps
// read-ahead memory while leaving enough depth to overlap reads with parsing.
constexpr int kReaderQueueDepth = 4;

} // namespace

DataManager::DataManager(QObject *parent) : QObject(parent) {}

DataManager::~DataManager() {
  // Stop and join the reader thread before our children (including the thread
  // object) are destroyed, so the read loop is not running at teardown.
  if (m_readerThread) {
    if (m_reader) {
      // Atomic flag only — safe to call from this thread; interrupts the read
      // loop. Thread-affine cleanup runs in the reader's destructor below,
      // after the thread is joined.
      m_reader->requestStop();
    }
    m_readerThread->quit();
    m_readerThread->wait();
  }
  // The reader was moved to the (now stopped) reader thread and has no parent,
  // so delete it explicitly.
  delete m_reader.data();
}

void DataManager::setDataReadConfig(const DataReadConfig &config) {
  m_dataReadConfig = config;
}

void DataManager::setSourceId(int id) { m_sourceId = id; }
int DataManager::getSourceId() const { return m_sourceId; }

void DataManager::start() {
  const QUrl url = m_dataReadConfig.url;
  const ChartEnums::DataFormat format = m_dataReadConfig.format;

  m_buffer = std::make_shared<DataBuffer>();
  m_buffer->setMaxRows(m_maxRows);
  // The reader has no parent so it can be moved to its own thread; the parser
  // and buffer updater stay on this manager's thread.
  m_reader = DataReaderFactory::create(url, nullptr);
  m_parser = DataParserFactory::create(format, this);
  m_bufferUpdater = new DataBufferUpdater(m_buffer, this);

  if (!m_reader || !m_parser) {
    delete m_reader.data();
    emit errorOccurred("DataManager::start: unsupported source or format");
    return;
  }

  m_readerThread = new QThread(this);
  m_reader->moveToThread(m_readerThread);

  // Run the read loop on the reader thread; stop() only flips an atomic flag so
  // a DirectConnection from this thread is safe and interrupts the loop at
  // once.
  connect(m_readerThread, &QThread::started, m_reader,
          &AbstractDataReader::start);
  // Two-part stop: requestStop() flips the atomic flag immediately (interrupts
  // a blocking read loop), while stop() runs on the reader thread to do any
  // thread-affine cleanup (e.g. closing a socket) and emit finished.
  connect(this, &DataManager::stopped, m_reader,
          &AbstractDataReader::requestStop, Qt::DirectConnection);
  connect(this, &DataManager::stopped, m_reader, &AbstractDataReader::stop,
          Qt::QueuedConnection);
  connect(m_reader, &AbstractDataReader::finished, m_readerThread,
          &QThread::quit, Qt::DirectConnection);

  connect(m_reader, &AbstractDataReader::dataReceived, this,
          &DataManager::onChunkReceived, Qt::QueuedConnection);
  connect(m_reader, &AbstractDataReader::errorOccurred, this,
          &DataManager::onErrorOccurred, Qt::QueuedConnection);

  connect(this, &DataManager::parseRequested, m_parser,
          &AbstractDataParser::parse, Qt::DirectConnection);

  if (auto *fastParser = qobject_cast<FastCsvDataParser *>(m_parser)) {
    // Fast CSV path: receive byte-span batches instead of QVariant rows, and
    // flush the final (possibly newline-less) record when the reader is done.
    // `done` is queued so it runs on this thread after every chunk is consumed.
    connect(fastParser, &FastCsvDataParser::batchParsed, this,
            &DataManager::onBatchParsed, Qt::DirectConnection);
    connect(m_reader, &AbstractDataReader::finished, fastParser,
            &FastCsvDataParser::done, Qt::QueuedConnection);
  } else {
    connect(m_parser, &AbstractDataParser::rowsParsed, this,
            &DataManager::onRowsParsed, Qt::DirectConnection);
  }

  connect(m_bufferUpdater, &DataBufferUpdater::errorOccurred, this,
          &DataManager::onErrorOccurred, Qt::DirectConnection);
  connect(m_bufferUpdater, &DataBufferUpdater::bufferUpdated, this,
          &DataManager::onBufferUpdated, Qt::DirectConnection);

  // PROFILING: ingest time (read + parse + buffer append). Read now overlaps
  // parsing on the reader thread; this fires after all chunks are consumed
  // because it is queued behind them on this thread's event loop.
  connect(m_reader, &AbstractDataReader::finished, this, [this] {
    const qint64 rows = m_buffer ? m_buffer->rowCount() : 0;
    CP_INFO(
        "[ingest] source {} finished: {} rows in {} ms (data-handler thread "
        "{:#x} — compare with the reader thread id above)",
        m_sourceId, rows, m_ingestTimer.elapsed(),
        reinterpret_cast<quintptr>(QThread::currentThreadId()));
    if (m_bufferUpdater) {
      m_bufferUpdater->logProfile();
    }
  });

  // Backpressure only applies to blocking static readers (file): a live stream
  // pushes chunks from its event handler and must not block, so leave it
  // unbounded there.
  const bool bounded = m_reader->mode() == ChartEnums::DataMode::Static;
  m_reader->setConfig(DataReaderConfig{
      .url = m_dataReadConfig.url,
      .chunkSize = m_dataReadConfig.chunkSize,
      .maxQueuedChunks = bounded ? kReaderQueueDepth : 0,
  });

  m_bufferUpdater->setConfig(DataBufferUpdaterConfig{
      .hasHeader = m_dataReadConfig.hasHeader,
      .skipRows = m_dataReadConfig.skipRows,
      .mode = m_reader->mode(),
      .columns = m_dataReadConfig.columns,
      .totalColumns = m_dataReadConfig.totalColumns,
  });

  m_ingestTimer.start();
  emit started();

  m_readerThread->start();
}

void DataManager::stop() { emit stopped(); }

void DataManager::setMaxRows(qint64 maxRows) {
  m_maxRows = maxRows;
  if (m_buffer) {
    m_buffer->setMaxRows(maxRows);
  }
}

void DataManager::onErrorOccurred(const QString &message) {
  emit errorOccurred(message);
}

void DataManager::onChunkReceived(const QByteArray &chunk) {
  // Runs on the data-handler thread, distinct from the reader thread that
  // produced the chunk — the thread ids printed here vs in the reader prove the
  // two stages run concurrently.
  CP_TRACE(
      "[data-handler|thread {:#x}] processing chunk ({} bytes) from reader",
      reinterpret_cast<quintptr>(QThread::currentThreadId()), chunk.size());

  // Parse + append synchronously on this thread, then free one queue slot so
  // the reader can produce the next chunk. The slot must be released exactly
  // once per received chunk, including on the early-out paths below.
  if (m_parser && !chunk.isEmpty()) {
    emit parseRequested(chunk);
  }

  if (m_reader) {
    m_reader->releaseChunkSlot();
  }
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

void DataManager::onBatchParsed(const CsvRowBatch &batch) {
  if (!m_bufferUpdater) {
    return;
  }

  if (batch.rowCount() == 0) {
    return;
  }

  m_bufferUpdater->parseBatch(batch);
}

void DataManager::onBufferUpdated() {
  if (!m_buffer) {
    return;
  }

  QMutexLocker locker(&m_snapshotMutex);
  m_currentSnapshot = std::move(m_buffer->snapshot());
}

const DataSnapshot &DataManager::getLatestSnapshot() {
  QMutexLocker locker(&m_snapshotMutex);
  return m_currentSnapshot;
}

} // namespace ChartPlotter

#include "ChartPlotter/data/reader/FileDataReader.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

#include <QElapsedTimer>
#include <QThread>

namespace ChartPlotter {

namespace {
constexpr qint64 DefaultChunkSize = 2 * ChartEnums::DataUnit::Mb;

quintptr currentThreadId() {
  return reinterpret_cast<quintptr>(QThread::currentThreadId());
}

} // namespace

FileDataReader::FileDataReader(QObject *parent) : AbstractDataReader(parent) {}

void FileDataReader::setConfig(const DataReaderConfig &config) {
  m_config = config;
  if (m_config.chunkSize <= 0) {
    emit errorOccurred(std::move(
        QStringLiteral("FileDataReader::setConfig: chunk size should not be "
                       "less or equal to 0, default to ") +
        QString::number(DefaultChunkSize)));
    m_config.chunkSize = DefaultChunkSize;
  }

  setQueueCapacity(m_config.maxQueuedChunks);

  m_configLoaded = true;
}

ChartEnums::DataMode FileDataReader::mode() const {
  return ChartEnums::DataMode::Static;
}

void FileDataReader::start() {
  if (m_running) {
    return;
  }

  if (!m_configLoaded) {
    fail(std::move(
        QStringLiteral("FileDataReader::start: config is not loaded")));
    return;
  }

  m_running = true;
  m_stopRequested.store(false);

  emit started();

  if (!m_config.url.isLocalFile()) {
    fail(std::move(
        QStringLiteral("FileDataReader::start: url is not a local file: ") +
        m_config.url.toString()));
    return;
  }

  const QString localPath = m_config.url.toLocalFile();

  if (localPath.isEmpty()) {
    fail(std::move(
        QStringLiteral("FileDataReader::start: empty local file path")));
    return;
  }

  QFile file(localPath);

  if (!file.open(QIODevice::ReadOnly)) {
    fail(std::move(
        QStringLiteral("FileDataReader::start: failed to open file at path ") +
        localPath + QStringLiteral(": ") + file.errorString()));
    return;
  }

  QElapsedTimer readTimer;
  readTimer.start();
  int chunkIndex = 0;
  qint64 totalBytes = 0;

  CP_INFO("[reader|thread {:#x}] begin reading '{}' (chunk size {} KB) — "
          "running off the data-handler/UI thread",
          currentThreadId(), localPath.toStdString(),
          m_config.chunkSize / ChartEnums::DataUnit::Kb);

  while (!file.atEnd()) {
    if (m_stopRequested.load()) {
      file.close();
      CP_INFO("[reader|thread {:#x}] stop requested — halting after {} chunks "
              "({} bytes)",
              currentThreadId(), chunkIndex, totalBytes);
      finishStopped();
      return;
    }

    QByteArray chunk = file.read(m_config.chunkSize);

    if (chunk.isEmpty()) {
      if (file.error() != QFile::NoError) {
        QString message =
            QStringLiteral(
                "FileDataReader::start: failed while reading file: ") +
            file.errorString();

        file.close();
        fail(std::move(message));
        return;
      }

      break;
    }

    ++chunkIndex;
    totalBytes += chunk.size();
    CP_INFO(
        "[reader|thread {:#x}] read chunk #{} ({} bytes, {} total) — handing "
        "off to data handler",
        currentThreadId(), chunkIndex, chunk.size(), totalBytes);

    // Backpressure: blocks here while the parser is behind and the queue is
    // full, returning early (without emitting) if a stop is requested. When the
    // next "read chunk" line stalls, the reader is waiting on the data handler.
    emitChunk(std::move(chunk));
  }

  CP_INFO(
      "[reader|thread {:#x}] done — read {} chunks, {} bytes in {} ms (read "
      "alone, overlapped with parsing on the data-handler thread)",
      currentThreadId(), chunkIndex, totalBytes, readTimer.elapsed());

  file.close();
  finishNormally();
}

void FileDataReader::stop() { m_stopRequested.store(true); }

void FileDataReader::finishNormally() {
  if (!m_running) {
    return;
  }

  m_running = false;
  emit finished();
}

void FileDataReader::finishStopped() {
  if (!m_running) {
    return;
  }

  m_running = false;
  emit stopped();
  emit finished();
}

void FileDataReader::fail(QString &&message) {
  if (!m_running) {
    emit errorOccurred(std::move(message));
    emit finished();
    return;
  }

  m_running = false;
  emit errorOccurred(std::move(message));
  emit finished();
}

} // namespace ChartPlotter

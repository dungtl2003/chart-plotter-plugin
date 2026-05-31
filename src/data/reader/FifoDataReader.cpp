#include "ChartPlotter/data/reader/FifoDataReader.hpp"

#include <QFileInfo>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ChartPlotter {

namespace {
constexpr qint64 DefaultChunkSize = 64 * ChartEnums::DataUnit::Kb;
}

FifoDataReader::FifoDataReader(QObject *parent) : AbstractDataReader(parent) {}

FifoDataReader::~FifoDataReader() { closeFifo(); }

void FifoDataReader::setConfig(const DataReaderConfig &config) {
  m_config = config;
  if (m_config.chunkSize <= 0) {
    emit errorOccurred("FifoDataReader::setConfig: chunk size should not be "
                       "less or equal to 0, default to " +
                       QString::number(DefaultChunkSize));
    m_config.chunkSize = DefaultChunkSize;
  }

  m_configLoaded = true;
}

ChartEnums::DataMode FifoDataReader::mode() const {
  return ChartEnums::DataMode::Realtime;
}

void FifoDataReader::start() {
  if (m_running) {
    return;
  }

  if (!m_configLoaded) {
    fail("FifoDataReader::start: config is not loaded");
    return;
  }

  if (!openFifo()) {
    return;
  }

  m_running = true;
  m_stopRequested.store(false);

  emit started();
}

void FifoDataReader::stop() {
  m_stopRequested.store(true);

  if (!m_running) {
    return;
  }

  finishStopped();
}

bool FifoDataReader::openFifo() {
  const QString path = m_config.url.path();

  if (path.isEmpty()) {
    fail("FifoDataReader::openFifo: invalid fifo url: " +
         m_config.url.toString());
    return false;
  }

  QFileInfo info(path);

  if (!info.exists()) {
    fail("FifoDataReader::openFifo: fifo does not exist: " + path);
    return false;
  }

  struct stat st{};

  if (::stat(path.toLocal8Bit().constData(), &st) != 0) {
    fail("FifoDataReader::openFifo: stat failed for " + path + ": " +
         QString::fromLocal8Bit(std::strerror(errno)));
    return false;
  }

  if (!S_ISFIFO(st.st_mode)) {
    fail("FifoDataReader::openFifo: path is not a FIFO: " + path);
    return false;
  }

  /*
   * Use O_RDWR instead of O_RDONLY.
   *
   * Why?
   * - O_RDONLY can block if no writer is connected.
   * - O_RDONLY | O_NONBLOCK can return EOF when writer closes.
   * - O_RDWR | O_NONBLOCK keeps the FIFO open from this process side,
   *   which makes repeated commands like:
   *
   *     echo "1,23.5" > /tmp/chartplotter.csvpipe
   *
   *   work more smoothly.
   */
  m_fd = ::open(path.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);

  if (m_fd < 0) {
    fail("FifoDataReader::openFifo: open failed for " + path + ": " +
         QString::fromLocal8Bit(std::strerror(errno)));
    return false;
  }

  m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);

  connect(m_notifier, &QSocketNotifier::activated, this,
          &FifoDataReader::onReadable);

  return true;
}

void FifoDataReader::onReadable() {
  if (!m_running) {
    return;
  }

  if (m_stopRequested.load()) {
    finishStopped();
    return;
  }

  if (m_fd < 0) {
    return;
  }

  /*
   * Disable while reading to avoid repeated activation while this slot
   * is still processing.
   */
  if (m_notifier) {
    m_notifier->setEnabled(false);
  }

  while (m_running && !m_stopRequested.load()) {
    QByteArray chunk;
    chunk.resize(static_cast<int>(m_config.chunkSize));

    const ssize_t n = ::read(m_fd, chunk.data(), chunk.size());

    if (n > 0) {
      chunk.resize(static_cast<int>(n));
      emit dataReceived(chunk);
      continue;
    }

    if (n == 0) {
      /*
       * No writer currently has data available.
       * Because we opened with O_RDWR, this should usually not mean final EOF.
       * Keep waiting.
       */
      break;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break;
    }

    const QString message = "FifoDataReader::onReadable: read failed: " +
                            QString::fromLocal8Bit(std::strerror(errno));

    if (m_notifier) {
      m_notifier->setEnabled(true);
    }

    fail(message);
    return;
  }

  if (m_notifier && m_running) {
    m_notifier->setEnabled(true);
  }
}

void FifoDataReader::closeFifo() {
  if (m_notifier) {
    m_notifier->setEnabled(false);
    m_notifier->deleteLater();
    m_notifier = nullptr;
  }

  if (m_fd >= 0) {
    ::close(m_fd);
    m_fd = -1;
  }
}

void FifoDataReader::finishStopped() {
  if (!m_running) {
    return;
  }

  closeFifo();

  m_running = false;

  emit stopped();
  emit finished();
}

void FifoDataReader::fail(const QString &message) {
  closeFifo();

  m_running = false;

  emit errorOccurred(message);
  emit finished();
}

} // namespace ChartPlotter

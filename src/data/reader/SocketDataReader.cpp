#include "ChartPlotter/data/reader/SocketDataReader.hpp"

namespace ChartPlotter {

namespace {
constexpr qint64 DefaultChunkSize = 64 * ChartEnums::DataUnit::Kb;
}

SocketDataReader::SocketDataReader(QObject *parent)
    : AbstractDataReader(parent), m_socket(new QTcpSocket(this)) {

  connect(m_socket, &QTcpSocket::connected, this,
          &SocketDataReader::onConnected);
  connect(m_socket, &QTcpSocket::disconnected, this,
          &SocketDataReader::onDisconnected);
  connect(m_socket, &QTcpSocket::readyRead, this,
          &SocketDataReader::onReadyRead);
  connect(m_socket, &QTcpSocket::errorOccurred, this,
          &SocketDataReader::onErrorOccurred);
}

SocketDataReader::~SocketDataReader() { cleanupSocket(); }

void SocketDataReader::setConfig(const DataReaderConfig &config) {
  m_config = config;

  if (m_config.chunkSize <= 0) {
    emit errorOccurred("SocketDataReader::setConfig: chunk size should not be "
                       "less or equal to 0, default to " +
                       QString::number(DefaultChunkSize));
    m_config.chunkSize = DefaultChunkSize;
  }

  m_configLoaded = true;
}

ChartEnums::DataMode SocketDataReader::mode() const {
  return ChartEnums::DataMode::Realtime;
}

void SocketDataReader::start() {
  if (m_running) {
    return;
  }

  if (!m_configLoaded) {
    fail("SocketDataReader::start: config is not loaded");
    return;
  }

  const QString host = m_config.url.host();
  const int port = m_config.url.port();

  if (host.isEmpty() || port <= 0) {
    fail("SocketDataReader::start: invalid socket url: " +
         m_config.url.toString());
    return;
  }

  m_running = true;
  m_socket->connectToHost(host, port);
}

void SocketDataReader::stop() {
  if (!m_running) {
    return;
  }

  m_running = false;
  cleanupSocket();

  emit stopped();
  emit finished();
}

void SocketDataReader::onConnected() { emit started(); }

void SocketDataReader::onDisconnected() {
  if (m_running) {
    m_running = false;
    emit stopped();
    emit finished();
  }
}

void SocketDataReader::onReadyRead() {
  if (!m_running) {
    return;
  }

  while (m_socket->bytesAvailable() > 0) {
    QByteArray chunk = m_socket->read(m_config.chunkSize);

    if (!chunk.isEmpty()) {
      emit dataReceived(chunk);
    }
  }
}

void SocketDataReader::onErrorOccurred(
    QAbstractSocket::SocketError /*socketError*/) {
  if (!m_running) {
    return;
  }

  fail("SocketDataReader::onErrorOccurred: " + m_socket->errorString());
}

void SocketDataReader::cleanupSocket() {
  if (m_socket->state() != QAbstractSocket::UnconnectedState) {
    m_socket->abort();
  }
}

void SocketDataReader::fail(const QString &message) {
  m_running = false;
  cleanupSocket();

  emit errorOccurred(message);
  emit finished();
}

} // namespace ChartPlotter

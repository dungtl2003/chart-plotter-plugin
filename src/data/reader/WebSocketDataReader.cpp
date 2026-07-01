#include "ChartPlotter/data/reader/WebSocketDataReader.hpp"

#include <QSslError>
#include <QThread>

namespace ChartPlotter {

WebSocketDataReader::WebSocketDataReader(QObject *parent)
    : AbstractDataReader(parent) {
  // The socket is created in start() (see the header note) so it lives on the
  // reader thread. Nothing to wire up here.
}

WebSocketDataReader::~WebSocketDataReader() { cleanupSocket(); }

void WebSocketDataReader::setConfig(const DataReaderConfig &config) {
  m_config = config;
  m_configLoaded = true;
}

ChartEnums::DataMode WebSocketDataReader::mode() const {
  return ChartEnums::DataMode::Realtime;
}

void WebSocketDataReader::start() {
  if (m_running) {
    return;
  }

  if (!m_configLoaded) {
    fail("WebSocketDataReader::start: config is not loaded");
    return;
  }

  if (!m_config.url.isValid()) {
    fail("WebSocketDataReader::start: invalid websocket url: " +
         m_config.url.toString());
    return;
  }

  // Create the socket here, on the reader thread, so its (and its internal
  // child sockets') thread affinity matches the thread that drives it. Parented
  // to this reader so it is destroyed with it.
  if (!m_webSocket) {
    m_webSocket =
        new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    connect(m_webSocket, &QWebSocket::connected, this,
            &WebSocketDataReader::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this,
            &WebSocketDataReader::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this,
            &WebSocketDataReader::onTextMessageReceived);
    connect(m_webSocket, &QWebSocket::binaryMessageReceived, this,
            &WebSocketDataReader::onBinaryMessageReceived);
    // Handle socket and SSL errors
    connect(m_webSocket, &QWebSocket::errorOccurred, this,
            &WebSocketDataReader::onErrorOccurred);
    connect(m_webSocket, &QWebSocket::sslErrors, this,
            &WebSocketDataReader::onSslErrors);

    // The reader thread outlives its event loop only briefly: DataManager
    // quits + wait()s the thread, then deletes this reader from another thread.
    // Delete the socket here (DirectConnection → runs on the reader thread as it
    // finishes) so its internal socket/notifiers are torn down on their own
    // thread, avoiding cross-thread-deletion warnings.
    connect(
        QThread::currentThread(), &QThread::finished, this,
        [this]() {
          if (m_webSocket) {
            m_webSocket->close();
            delete m_webSocket;
            m_webSocket = nullptr;
          }
        },
        Qt::DirectConnection);
  }

  m_running = true;
  m_webSocket->open(m_config.url);
}

void WebSocketDataReader::stop() {
  if (!m_running) {
    return;
  }

  m_running = false;
  cleanupSocket();

  emit stopped();
  emit finished();
}

void WebSocketDataReader::onConnected() { emit started(); }

void WebSocketDataReader::onDisconnected() {
  if (m_running) {
    m_running = false;
    emit stopped();
    emit finished();
  }
}

void WebSocketDataReader::onTextMessageReceived(const QString &message) {
  if (!m_running) {
    return;
  }

  // Convert the JSON string to QByteArray for your interface
  emit dataReceived(message.toUtf8());
}

void WebSocketDataReader::onBinaryMessageReceived(const QByteArray &message) {
  if (!m_running) {
    return;
  }

  emit dataReceived(message);
}

void WebSocketDataReader::onErrorOccurred(
    QAbstractSocket::SocketError /*error*/) {
  if (!m_running) {
    return;
  }

  fail("WebSocketDataReader::onErrorOccurred: " +
       (m_webSocket ? m_webSocket->errorString() : QStringLiteral("socket")));
}

void WebSocketDataReader::onSslErrors(const QList<QSslError> &errors) {
  QString errorMessages;
  for (const QSslError &error : errors) {
    errorMessages += error.errorString() + "; ";
  }

  fail("WebSocketDataReader::onSslErrors: " + errorMessages);
}

void WebSocketDataReader::cleanupSocket() {
  if (m_webSocket && m_webSocket->isValid()) {
    m_webSocket->close();
  }
}

void WebSocketDataReader::fail(const QString &message) {
  m_running = false;
  cleanupSocket();

  emit errorOccurred(message);
  emit finished();
}

} // namespace ChartPlotter

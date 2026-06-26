#include "ChartPlotter/data/reader/WebSocketDataReader.hpp"

#include <QSslError>

namespace ChartPlotter {

WebSocketDataReader::WebSocketDataReader(QObject *parent)
    : AbstractDataReader(parent) {

  connect(&m_webSocket, &QWebSocket::connected, this,
          &WebSocketDataReader::onConnected);
  connect(&m_webSocket, &QWebSocket::disconnected, this,
          &WebSocketDataReader::onDisconnected);

  connect(&m_webSocket, &QWebSocket::textMessageReceived, this,
          &WebSocketDataReader::onTextMessageReceived);

  connect(&m_webSocket, &QWebSocket::binaryMessageReceived, this,
          &WebSocketDataReader::onBinaryMessageReceived);

  // Handle socket and SSL errors
  connect(&m_webSocket, &QWebSocket::errorOccurred, this,
          &WebSocketDataReader::onErrorOccurred);
  connect(&m_webSocket, &QWebSocket::sslErrors, this,
          &WebSocketDataReader::onSslErrors);
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

  m_running = true;
  m_webSocket.open(m_config.url);
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

  fail("WebSocketDataReader::onErrorOccurred: " + m_webSocket.errorString());
}

void WebSocketDataReader::onSslErrors(const QList<QSslError> &errors) {
  QString errorMessages;
  for (const QSslError &error : errors) {
    errorMessages += error.errorString() + "; ";
  }

  fail("WebSocketDataReader::onSslErrors: " + errorMessages);
}

void WebSocketDataReader::cleanupSocket() {
  if (m_webSocket.isValid()) {
    m_webSocket.close();
  }
}

void WebSocketDataReader::fail(const QString &message) {
  m_running = false;
  cleanupSocket();

  emit errorOccurred(message);
  emit finished();
}

} // namespace ChartPlotter

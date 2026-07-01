#pragma once

#include "ChartPlotter/data/reader/AbstractDataReader.hpp"

#include <QWebSocket>

namespace ChartPlotter {

class WebSocketDataReader : public AbstractDataReader {
  Q_OBJECT

public:
  explicit WebSocketDataReader(QObject *parent = nullptr);
  ~WebSocketDataReader() override;

  void setConfig(const DataReaderConfig &config) override;
  ChartEnums::DataMode mode() const override;

public slots:
  void start() override;
  void stop() override;

private slots:
  void onConnected();
  void onDisconnected();
  void onTextMessageReceived(const QString &message);
  void onBinaryMessageReceived(const QByteArray &message);
  void onErrorOccurred(QAbstractSocket::SocketError error);
  void onSslErrors(const QList<QSslError> &errors);

private:
  DataReaderConfig m_config;
  // Created lazily in start() (which runs on the reader thread) and parented to
  // this reader, so its thread affinity matches the thread that drives it. A
  // value member would be constructed on the creating thread and NOT moved by
  // moveToThread() (it is not a QObject child), which crashes when open() then
  // creates child sockets on a different thread.
  QWebSocket *m_webSocket = nullptr;
  bool m_running = false;
  bool m_configLoaded = false;

  void cleanupSocket();
  void fail(const QString &message);
};

} // namespace ChartPlotter

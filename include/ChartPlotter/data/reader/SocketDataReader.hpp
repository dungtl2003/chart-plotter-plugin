#pragma once

#include "ChartPlotter/data/reader/AbstractDataReader.hpp"

#include <QTcpSocket>

namespace ChartPlotter {

class SocketDataReader : public AbstractDataReader {
  Q_OBJECT

public:
  explicit SocketDataReader(QObject *parent = nullptr);
  ~SocketDataReader() override;

  void setConfig(const DataReaderConfig &config) override;
  ChartEnums::DataMode mode() const override;

public slots:
  void start() override;
  void stop() override;

private slots:
  void onConnected();
  void onDisconnected();
  void onReadyRead();
  void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
  DataReaderConfig m_config;
  QTcpSocket *m_socket = nullptr;
  bool m_running = false;
  bool m_configLoaded = false;

  void cleanupSocket();
  void fail(const QString &message);
};

} // namespace ChartPlotter

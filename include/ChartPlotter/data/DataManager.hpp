#pragma once

#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/data/DataBufferUpdater.hpp"
#include "ChartPlotter/data/DataReadConfig.hpp"
#include "ChartPlotter/data/parser/AbstractDataParser.hpp"
#include "ChartPlotter/data/reader/AbstractDataReader.hpp"

#include <spdlog/spdlog.h>

namespace ChartPlotter {

class DataManager : public QObject {
  Q_OBJECT

public:
  explicit DataManager(QObject *parent = nullptr);

  void setDataReadConfig(const DataReadConfig &config);

public slots:
  void start();
  void stop();

signals:
  void started();
  void stopped();
  void finished();
  void errorOccurred(QString message);
  void parseRequested(QByteArray chunk);
  void snapshotReady(DataSnapshot snapshot);

private slots:
  void onErrorOccurred(const QString &message);
  void onChunkReceived(const QByteArray &chunk);
  void onRowsParsed(const QVector<DataRow> &rows);
  void onBufferUpdated();

private:
  DataReadConfig m_dataReadConfig;
  std::shared_ptr<DataBuffer> m_buffer;
  QPointer<DataBufferUpdater> m_bufferUpdater;
  QPointer<AbstractDataReader> m_reader;
  QPointer<AbstractDataParser> m_parser;
};

} // namespace ChartPlotter

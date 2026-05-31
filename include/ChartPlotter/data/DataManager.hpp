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
  const QPointer<DataBuffer> buffer() const;

public slots:
  void start();
  void stop();

signals:
  void started();
  void stopped();
  void finished();
  void errorOccurred(const QString &message);
  void parseRequested(const QByteArray &chunk);

private slots:
  void onErrorOccurred(const QString &message);
  void onChunkReceived(const QByteArray &chunk);
  void onRowsParsed(const QVector<DataRow> &rows);
  void onReaderFinished();
  void onParserFinished();

private:
  DataReadConfig m_dataReadConfig;
  QPointer<DataBuffer> m_buffer;
  QPointer<DataBufferUpdater> m_bufferUpdater;
  QPointer<AbstractDataReader> m_reader;
  QPointer<AbstractDataParser> m_parser;
  bool m_readerDone = false;
  bool m_parserDone = false;

  void cleanup();
};

} // namespace ChartPlotter

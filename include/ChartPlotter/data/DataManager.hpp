#pragma once

#include "ChartPlotter/data/CsvRowBatch.hpp"
#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/data/DataBufferUpdater.hpp"
#include "ChartPlotter/data/DataReadConfig.hpp"
#include "ChartPlotter/data/parser/AbstractDataParser.hpp"
#include "ChartPlotter/data/reader/AbstractDataReader.hpp"

#include <spdlog/spdlog.h>

#include <QElapsedTimer>
#include <QThread>

namespace ChartPlotter {

class DataManager : public QObject {
  Q_OBJECT

public:
  explicit DataManager(QObject *parent = nullptr);
  ~DataManager() override;

  void setDataReadConfig(const DataReadConfig &config);
  void setSourceId(int id);
  int getSourceId() const;
  const DataSnapshot &getLatestSnapshot();

public slots:
  void start();
  void stop();
  // Forwarded to the DataBuffer. Invoked queued from the GUI thread so it runs
  // on this manager's worker thread.
  void setMaxRows(qint64 maxRows);

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
  void onBatchParsed(const CsvRowBatch &batch);
  void onBufferUpdated();

private:
  int m_sourceId = -1;
  qint64 m_maxRows = -1; // row cap forwarded to the buffer; <= 0 = unlimited
  DataReadConfig m_dataReadConfig;
  std::shared_ptr<DataBuffer> m_buffer;
  QPointer<DataBufferUpdater> m_bufferUpdater;
  QPointer<AbstractDataReader> m_reader;
  QPointer<AbstractDataParser> m_parser;
  // The reader runs on its own thread so file/socket I/O overlaps with parse +
  // buffer append. Owned by this manager; stopped and joined in the destructor.
  QPointer<QThread> m_readerThread;

  std::mutex m_snapshotMutex;
  DataSnapshot m_currentSnapshot;

  // Coarse ingest profiling: wall time from start() to reader finished.
  QElapsedTimer m_ingestTimer;
};

} // namespace ChartPlotter

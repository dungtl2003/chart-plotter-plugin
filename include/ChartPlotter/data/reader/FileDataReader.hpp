#pragma once

#include "ChartPlotter/data/reader/AbstractDataReader.hpp"

#include <QFile>
#include <QUrl>

#include <atomic>

namespace ChartPlotter {

class FileDataReader : public AbstractDataReader {
  Q_OBJECT

public:
  explicit FileDataReader(QObject *parent = nullptr);

  void setConfig(const DataReaderConfig &url) override;
  ChartEnums::DataMode mode() const override;

public slots:
  void start() override;
  void stop() override;

private:
  void finishNormally();
  void finishStopped();
  void fail(QString &&message);

private:
  DataReaderConfig m_config;

  bool m_running = false;
  bool m_configLoaded = false;
  std::atomic_bool m_stopRequested = false;
};

} // namespace ChartPlotter

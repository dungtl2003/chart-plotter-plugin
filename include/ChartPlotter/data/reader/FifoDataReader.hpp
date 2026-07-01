#pragma once

#include "ChartPlotter/data/reader/AbstractDataReader.hpp"

#include <QSocketNotifier>
#include <QUrl>

namespace ChartPlotter {

class FifoDataReader : public AbstractDataReader {
  Q_OBJECT

public:
  explicit FifoDataReader(QObject *parent = nullptr);
  ~FifoDataReader() override;

  void setConfig(const DataReaderConfig &config) override;
  ChartEnums::DataMode mode() const override;

public slots:
  void start() override;
  void stop() override;

private slots:
  void onReadable();

private:
  DataReaderConfig m_config;
  int m_fd = -1;
  QSocketNotifier *m_notifier = nullptr;
  bool m_running = false;
  bool m_configLoaded = false;

  bool openFifo();
  void closeFifo();
  void finishStopped();
  void fail(const QString &message);
};

} // namespace ChartPlotter

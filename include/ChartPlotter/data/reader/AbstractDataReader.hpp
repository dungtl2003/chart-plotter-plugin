#pragma once

#include "ChartPlotter/types/ChartEnums.hpp"

#include <QObject>
#include <QString>
#include <QtQml>

namespace ChartPlotter {

struct DataReaderConfig {
  QUrl url;
  qint64 chunkSize = 2 * ChartEnums::DataUnit::Mb;
};

class AbstractDataReader : public QObject {
  Q_OBJECT
  QML_UNCREATABLE("AbstractChart is a base class and cannot be instantiated.")

public:
  explicit AbstractDataReader(QObject *parent = nullptr);

  explicit AbstractDataReader(const AbstractDataReader &) = delete;
  explicit AbstractDataReader(AbstractDataReader &&) = delete;
  AbstractDataReader &operator=(const AbstractDataReader &) = delete;
  AbstractDataReader &operator=(AbstractDataReader &&) = delete;
  virtual ~AbstractDataReader() = default;

  virtual void setConfig(const DataReaderConfig &config) = 0;
  virtual ChartEnums::DataMode mode() const = 0;

public slots:
  virtual void start() = 0;
  virtual void stop() = 0;

signals:
  void dataReceived(QByteArray data);
  void started();
  void finished();
  void stopped();
  void errorOccurred(QString message);
};

} // namespace ChartPlotter

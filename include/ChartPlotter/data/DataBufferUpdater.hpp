#pragma once

#include "ChartPlotter/data/DataBuffer.hpp"
#include "DataReadConfig.hpp"
#include "types/ChartEnums.hpp"

#include <expected>

namespace ChartPlotter {

struct DataBufferUpdaterConfig {
  bool hasHeader = false;
  int skipRows = 0;
  ChartEnums::DataMode mode = ChartEnums::DataMode::Static;
  QVector<DataColumnConfig> columns;
  qint64 totalColumns = -1;
};

class DataBufferUpdater : public QObject {
  Q_OBJECT

public:
  DataBufferUpdater(std::shared_ptr<DataBuffer> buffer,
                    QObject *parent = nullptr);

  void setConfig(const DataBufferUpdaterConfig &config);

public slots:
  void reset();
  void parseRows(const QVector<DataRow> &rows);

signals:
  void errorOccurred(QString message);
  void bufferUpdated();

private:
  std::shared_ptr<DataBuffer> m_buffer;
  DataBufferUpdaterConfig m_config;
  bool m_canInferTypes = false;
  bool m_configLoaded = false;
  bool m_firstRowParsed = false;
  qint64 m_columnSize = -1;
  qint64 m_currentPhysicalRow = 0;

  std::expected<void, std::string> parseRow(const DataRow &row);
  std::expected<void, std::string> parseHeaderRow(const DataRow &headerRow);
  bool rowIsValid(const DataRow &row);
  bool convertValue(MutableDataColumn &col, const QVariant &val,
                    double &outValue);
  void fail(QString &&message);
};

} // namespace ChartPlotter

#pragma once

#include "ChartPlotter/data/CsvRowBatch.hpp"
#include "ChartPlotter/data/DataBuffer.hpp"
#include "DataReadConfig.hpp"
#include "types/ChartEnums.hpp"

#include <QElapsedTimer>

#include <expected>
#include <string_view>

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

  // PROFILING: dump accumulated convert vs append time (nanoseconds totals).
  void logProfile() const;

public slots:
  void reset();
  void parseRows(const QVector<DataRow> &rows);
  // Fast path: consume byte-span records and convert directly to double.
  void parseBatch(const CsvRowBatch &batch);

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
  // Span variant of convertValue: numeric fields go straight through
  // std::from_chars with no QString/QVariant allocation.
  bool convertSpanValue(MutableDataColumn &col, std::string_view field,
                        double &outValue);
  void fail(QString &&message);

  // Fused columnar fast path: column pointers resolved once (columns are fixed
  // after the header), and per-column staging buffers reused across batches.
  void prepareFastStaging();
  std::vector<MutableDataColumn *> m_colPtrs;
  QVector<QVector<double>> m_colValues;

  // PROFILING accumulators (nanoseconds), filled by parseBatch.
  QElapsedTimer m_profTimer;
  qint64 m_convertNs = 0;
  qint64 m_appendNs = 0;
};

} // namespace ChartPlotter

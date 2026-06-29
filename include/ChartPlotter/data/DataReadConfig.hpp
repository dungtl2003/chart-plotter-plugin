#pragma once

#include "ChartPlotter/types/ChartEnums.hpp"

#include <QUrl>

namespace ChartPlotter {

struct DataColumnConfig {
  int idx;
  QString name;
  ChartEnums::DataType type = ChartEnums::DataType::Unknown;
};

struct DataReadConfig {
  qint64 totalColumns = -1;
  QUrl url;
  ChartEnums::DataFormat format = ChartEnums::DataFormat::Unknown;
  qint64 chunkSize = 2 * ChartEnums::DataUnit::Mb;
  bool hasHeader = false;
  int skipRows = 0;

  QVector<DataColumnConfig> columns;
};

} // namespace ChartPlotter

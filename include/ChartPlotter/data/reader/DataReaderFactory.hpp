#pragma once

#include "ChartPlotter/data/reader/AbstractDataReader.hpp"

namespace ChartPlotter {

class DataReaderFactory {
public:
  DataReaderFactory() = default;
  static AbstractDataReader *create(const QUrl &url, QObject *parent);
};

} // namespace ChartPlotter

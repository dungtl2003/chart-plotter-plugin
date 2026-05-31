#pragma once

#include "ChartPlotter/data/parser/AbstractDataParser.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"

namespace ChartPlotter {

class DataParserFactory {
public:
  DataParserFactory() = default;
  static AbstractDataParser *create(const ChartEnums::DataFormat &format,
                                    QObject *parent);
};

} // namespace ChartPlotter

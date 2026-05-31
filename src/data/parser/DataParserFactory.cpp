#include "ChartPlotter/data/parser/DataParserFactory.hpp"
#include "ChartPlotter/data/parser/CsvDataParser.hpp"

namespace ChartPlotter {

AbstractDataParser *
DataParserFactory::create(const ChartEnums::DataFormat &format,
                          QObject *parent) {
  switch (format) {
  case ChartEnums::DataFormat::Csv:
    return new CsvDataParser(parent);
  default:
    return nullptr;
  }
}

} // namespace ChartPlotter

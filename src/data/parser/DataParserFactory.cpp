#include "ChartPlotter/data/parser/DataParserFactory.hpp"
#include "ChartPlotter/data/parser/BinanceBasicJsonDataParser.hpp"
#include "ChartPlotter/data/parser/CsvDataParser.hpp"

namespace ChartPlotter {

AbstractDataParser *
DataParserFactory::create(const ChartEnums::DataFormat &format,
                          QObject *parent) {
  switch (format) {
  case ChartEnums::DataFormat::Csv:
    return new CsvDataParser(parent);
  case ChartEnums::DataFormat::BinanceBasicJson:
    return new BinanceBasicJsonDataParser(parent);
  default:
    return nullptr;
  }
}

} // namespace ChartPlotter

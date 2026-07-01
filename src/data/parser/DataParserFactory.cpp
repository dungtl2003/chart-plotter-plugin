#include "ChartPlotter/data/parser/DataParserFactory.hpp"
#include "ChartPlotter/data/parser/BinanceBasicJsonDataParser.hpp"
#include "ChartPlotter/data/parser/BinanceTickerJsonDataParser.hpp"
#include "ChartPlotter/data/parser/BinanceTradeJsonDataParser.hpp"
#include "ChartPlotter/data/parser/FastCsvDataParser.hpp"

namespace ChartPlotter {

AbstractDataParser *
DataParserFactory::create(const ChartEnums::DataFormat &format,
                          QObject *parent) {
  switch (format) {
  case ChartEnums::DataFormat::Csv:
    return new FastCsvDataParser(parent);
  case ChartEnums::DataFormat::BinanceBasicJson:
    return new BinanceBasicJsonDataParser(parent);
  case ChartEnums::DataFormat::BinanceTickerJson:
    return new BinanceTickerJsonDataParser(parent);
  case ChartEnums::DataFormat::BinanceTradeJson:
    return new BinanceTradeJsonDataParser(parent);
  default:
    return nullptr;
  }
}

} // namespace ChartPlotter

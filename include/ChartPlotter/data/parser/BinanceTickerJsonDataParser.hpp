#include "ChartPlotter/data/parser/AbstractDataParser.hpp"

namespace ChartPlotter {

// Parser for Binance's individual-symbol 24hr ticker stream
// (`<symbol>@ticker`, e.g. wss://stream.binance.com:9443/ws/btcusdt@ticker).
// The payload is a single flat JSON object; every symbol (BTCUSDT, ETHUSDT,
// SOLUSDT, ...) returns the same shape, so one parser handles all of them.
class BinanceTickerJsonDataParser : public AbstractDataParser {
  Q_OBJECT

public:
  explicit BinanceTickerJsonDataParser(QObject *parent = nullptr);

  void parse(const QByteArray &chunk) override;
  void reset() override;
};

} // namespace ChartPlotter

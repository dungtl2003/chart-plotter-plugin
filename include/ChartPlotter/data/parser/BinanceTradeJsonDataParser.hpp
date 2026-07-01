#include "ChartPlotter/data/parser/AbstractDataParser.hpp"

namespace ChartPlotter {

// Parser for Binance's raw trade stream (`<symbol>@trade`, e.g.
// wss://stream.binance.com:9443/ws/btcusdt@trade). One message per executed
// trade (many per second on liquid markets), a single flat JSON object. Every
// symbol/market returns the same shape, so one parser serves all of them.
class BinanceTradeJsonDataParser : public AbstractDataParser {
  Q_OBJECT

public:
  explicit BinanceTradeJsonDataParser(QObject *parent = nullptr);

  void parse(const QByteArray &chunk) override;
  void reset() override;
};

} // namespace ChartPlotter

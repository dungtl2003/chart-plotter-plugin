#include "ChartPlotter/data/parser/BinanceTickerJsonDataParser.hpp"

#include <QJsonDocument>
#include <QJsonObject>

namespace ChartPlotter {

BinanceTickerJsonDataParser::BinanceTickerJsonDataParser(QObject *parent)
    : AbstractDataParser(parent) {}

/**
 * Individual symbol 24hr ticker payload (flat JSON object). Example:
 * {
 *   "e": "24hrTicker", "E": 1672515782136, "s": "BTCUSDT",
 *   "p": "0.0015", "P": "250.00", "w": "0.0018", "x": "0.0009",
 *   "c": "0.0025", "Q": "10", "b": "0.0024", "B": "10",
 *   "a": "0.0026", "A": "100", "o": "0.0010", "h": "0.0025",
 *   "l": "0.0010", "v": "10000", "q": "18",
 *   "O": 0, "C": 86400000, "F": 0, "L": 18150, "n": 18151
 * }
 *
 * Flattened to columns (index → key → meaning):
 *   0  e  Event type            12 A  Best ask quantity
 *   1  E  Event time (ms)       13 o  Open price
 *   2  s  Symbol                14 h  High price
 *   3  p  Price change          15 l  Low price
 *   4  P  Price change percent  16 v  Base asset volume
 *   5  w  Weighted avg price    17 q  Quote asset volume
 *   6  x  Prev-window price     18 O  Statistics open time
 *   7  c  Last (close) price    19 C  Statistics close time
 *   8  Q  Last quantity         20 F  First trade ID
 *   9  b  Best bid price        21 L  Last trade ID
 *   10 B  Best bid quantity     22 n  Total number of trades
 *   11 a  Best ask price
 *
 * The numeric fields arrive as JSON strings ("0.0025"); the buffer coerces
 * them to double. For a price chart use x = column 1 (E, declared as Date) and
 * y = column 7 (c, last price).
 */
void BinanceTickerJsonDataParser::parse(const QByteArray &chunk) {
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(chunk, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    emit errorOccurred("JSON Parse Error: " + parseError.errorString());
    return;
  }

  if (!doc.isObject()) {
    emit errorOccurred(
        "Expected a JSON object, but received a different format.");
    return;
  }

  const QJsonObject jsonObj = doc.object();

  // Column order is fixed (see the doc comment above). Keep it stable so the
  // xColumn/yColumn indices in QML stay valid.
  static const QStringList keys = {"e", "E", "s", "p", "P", "w", "x", "c",
                                   "Q", "b", "B", "a", "A", "o", "h", "l",
                                   "v", "q", "O", "C", "F", "L", "n"};

  DataRow row;
  row.values.reserve(keys.size());
  for (const QString &key : keys) {
    row.append(jsonObj.value(key).toVariant());
  }

  QVector<DataRow> rows;
  rows.append(std::move(row));
  emit rowsParsed(std::move(rows));
}

void BinanceTickerJsonDataParser::reset() {
  // Stateless: each WebSocket frame is a complete JSON object parsed in place.
}

} // namespace ChartPlotter

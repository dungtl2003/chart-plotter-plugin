#include "ChartPlotter/data/parser/BinanceBasicJsonDataParser.hpp"

namespace ChartPlotter {

BinanceBasicJsonDataParser::BinanceBasicJsonDataParser(QObject *parent)
    : AbstractDataParser(parent) {}

/**
 * {'e': 'kline', 'E': 1782379374015, 's': 'BTCUSDT', 'k': {'t': 1782379373000,
 * 'T': 1782379373999, 's': 'BTCUSDT', 'i': '1s', 'f': 6442673083, 'L':
 * 6442673452, 'o': '61640.29000000', 'c': '61626.00000000', 'h':
 * '61640.29000000', 'l': '61626.00000000', 'v
 * ': '0.66877000', 'n': 370, 'x': True, 'q': '41217.93733810', 'V':
 * '0.00000000', 'Q': '0.00000000', 'B': '0'}}
 *
 * We will convert to columns:
 * e, E, s, t, T, i, f, L, o, c, h, l, v, n, x, q, V, Q, B
 *
 * With:
 * e: Event type
 * E: Event time
 * s: Symbol
 * t: Kline start time
 * T: Kline close time
 * i: Interval
 * f: First trade ID
 * L: Last trade ID
 * o: Open price
 * c: Close price
 * h: High price
 * l: Low price
 * v: Base asset volume
 * n: Number of trades
 * x: Is this kline closed?
 * q: Quote asset volume
 * V: Taker buy base asset volume
 * Q: Taker buy quote asset volume
 * B: Ignore
 */
void BinanceBasicJsonDataParser::parse(const QByteArray &chunk) {
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

  QJsonObject jsonObj = doc.object();

  // Safely extract the nested 'k' object.
  // If it's missing, this returns an empty QJsonObject,
  // which will just yield invalid QVariants for those fields.
  QJsonObject klineObj = jsonObj.value("k").toObject();

  DataRow row;

  // Split the keys based on where they live in the JSON hierarchy
  const QStringList rootKeys = {"e", "E", "s"};
  const QStringList klineKeys = {"t", "T", "i", "f", "L", "o", "c", "h",
                                 "l", "v", "n", "x", "q", "V", "Q", "B"};

  // Append root-level data
  for (const QString &key : rootKeys) {
    row.append(jsonObj.value(key).toVariant());
  }

  // Append nested kline data
  for (const QString &key : klineKeys) {
    row.append(klineObj.value(key).toVariant());
  }

  QVector<DataRow> rows;
  rows.append(row);
  emit rowsParsed(std::move(rows));
}

void BinanceBasicJsonDataParser::reset() {
  // This basic version processes discrete, fully-formed chunks immediately.
  // Unlike a stream-based parser, there is no internal buffer or state machine
  // to clear.
}

} // namespace ChartPlotter

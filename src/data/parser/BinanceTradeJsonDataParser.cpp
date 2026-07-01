#include "ChartPlotter/data/parser/BinanceTradeJsonDataParser.hpp"

#include <QJsonDocument>
#include <QJsonObject>

namespace ChartPlotter {

BinanceTradeJsonDataParser::BinanceTradeJsonDataParser(QObject *parent)
    : AbstractDataParser(parent) {}

/**
 * Raw trade payload (flat JSON object). Example:
 * {
 *   "e": "trade", "E": 1672515782136, "s": "BTCUSDT", "t": 12345,
 *   "p": "63000.00", "q": "0.01", "T": 1672515782136, "m": true, "M": true
 * }
 *
 * Flattened to columns (index → key → meaning):
 *   0 e  Event type              5 q  Quantity
 *   1 E  Event time (ms)         6 T  Trade time (ms)
 *   2 s  Symbol                  7 m  Buyer is market maker?
 *   3 t  Trade ID                8 M  Ignore
 *   4 p  Price
 *
 * Price arrives as a JSON string ("63000.00"); the buffer coerces it to double.
 * For a price chart use x = column 1 (E, declared as Date) and y = column 4 (p).
 */
void BinanceTradeJsonDataParser::parse(const QByteArray &chunk) {
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

  // Fixed column order (see the doc comment); keep it stable so the QML
  // xColumn/yColumn indices stay valid.
  static const QStringList keys = {"e", "E", "s", "t", "p",
                                   "q", "T", "m", "M"};

  DataRow row;
  row.values.reserve(keys.size());
  for (const QString &key : keys) {
    row.append(jsonObj.value(key).toVariant());
  }

  QVector<DataRow> rows;
  rows.append(std::move(row));
  emit rowsParsed(std::move(rows));
}

void BinanceTradeJsonDataParser::reset() {
  // Stateless: each WebSocket frame is a complete JSON object parsed in place.
}

} // namespace ChartPlotter

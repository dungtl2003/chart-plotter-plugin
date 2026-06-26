#include "ChartPlotter/data/parser/AbstractDataParser.hpp"

namespace ChartPlotter {

class BinanceBasicJsonDataParser : public AbstractDataParser {
  Q_OBJECT

public:
  explicit BinanceBasicJsonDataParser(QObject *parent = nullptr);

  /**
   * This is the basic version, we assume the data is a valid JSON object.
   */
  void parse(const QByteArray &chunk) override;
  void reset() override;
};

} // namespace ChartPlotter

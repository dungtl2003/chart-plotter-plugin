#include "ChartPlotter/chart/AbstractChart.hpp"

class LineChart : public AbstractChart {
  Q_OBJECT
  QML_ELEMENT

public:
  explicit LineChart(QObject *parent = nullptr);

  ChartType type() const override;
};

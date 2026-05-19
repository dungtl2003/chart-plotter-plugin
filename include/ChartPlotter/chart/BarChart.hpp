#include "ChartPlotter/chart/AbstractChart.hpp"

class BarChart : public AbstractChart {
  Q_OBJECT
  QML_ELEMENT

public:
  explicit BarChart(QObject *parent = nullptr);

  ChartType type() const override;
};

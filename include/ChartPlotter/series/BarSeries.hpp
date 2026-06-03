#pragma once

#include "ChartPlotter/series/XYSeries.hpp"

namespace ChartPlotter {

class BarSeries : public XYSeries {
  Q_OBJECT
  QML_ELEMENT

public:
  explicit BarSeries(QObject *parent = nullptr);

  ChartEnums::SeriesType type() override;
};

} // namespace ChartPlotter

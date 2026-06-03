#pragma once

#include "ChartPlotter/series/AbstractSeries.hpp"

#include <QString>

namespace ChartPlotter {

enum class SeriesLayoutType {
  Unknown,
  XY,
  Pie,
};

struct ChartLayoutPlan {

  bool valid = false;
  QString errorMessage;

  SeriesLayoutType layoutType = SeriesLayoutType::Unknown;
  QVector<int> xySeriesIndexes;
  QVector<int> pieSeriesIndexes;
};

class ChartLayoutPlanner {

public:
  static ChartLayoutPlan
  buildPlan(const QVector<QPointer<AbstractSeries>> &series);

private:
  static SeriesLayoutType layoutTypeOf(const AbstractSeries *series);
};

} // namespace ChartPlotter

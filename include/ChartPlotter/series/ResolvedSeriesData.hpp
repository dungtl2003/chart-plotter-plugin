#pragma once

#include "ChartPlotter/types/ChartEnums.hpp"

#include <QString>

namespace ChartPlotter {

enum class ResolvedSeriesKind { XY, Pie, Unknown };

struct ResolvedSeriesData {
  ResolvedSeriesKind kind = ResolvedSeriesKind::Unknown;

  int seriesIndex = -1;
  int sourceId = -1;

  bool valid = false;
  QString errorMessage;

  // XY series
  QString xColumnName;
  QString yColumnName;

  int xColumnIndex = -1;
  int yColumnIndex = -1;

  ChartEnums::DataType xColumnType = ChartEnums::DataType::Unknown;
  ChartEnums::DataType yColumnType = ChartEnums::DataType::Unknown;

  // Pie series
  QString labelColumnName;
  QString valueColumnName;

  int labelColumnIndex = -1;
  int valueColumnIndex = -1;

  ChartEnums::DataType labelColumnType = ChartEnums::DataType::Unknown;
  ChartEnums::DataType valueColumnType = ChartEnums::DataType::Unknown;
};

struct SeriesResolveResult {
  bool valid = false;
  QString errorMessage;

  QVector<ResolvedSeriesData> xySeries;
  QVector<ResolvedSeriesData> pieSeries;

  QString sharedXColumnName;
  ChartEnums::DataType sharedXColumnType = ChartEnums::DataType::Unknown;
};

} // namespace ChartPlotter

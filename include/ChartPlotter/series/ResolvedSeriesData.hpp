#pragma once

#include "ChartPlotter/axis/CategoryAxis.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"
#include "ChartPlotter/types/DataRange.hpp"

#include <QString>
#include <QVector>

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

  qint64 xColumnIndex = -1;
  qint64 yColumnIndex = -1;

  ChartEnums::DataType xColumnType = ChartEnums::DataType::Unknown;
  ChartEnums::DataType yColumnType = ChartEnums::DataType::Unknown;

  DataRange absoluteXRange;
  DataRange absoluteYRange;

  // Maps the local categorical 'double' ID in the DataChunk
  // to the global CategoryAxis 'double' ID.
  QVector<double> localToGlobalXMap;

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
  CategoryAxis
      sharedXCategories; // populated only when sharedXColumnType == String

  DataRange absoluteXRange;
  DataRange absoluteYRange;
};

} // namespace ChartPlotter

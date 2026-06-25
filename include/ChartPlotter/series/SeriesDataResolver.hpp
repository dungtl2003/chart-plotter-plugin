#pragma once

#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/series/AbstractSeries.hpp"
#include "ChartPlotter/series/PieSeries.hpp"
#include "ChartPlotter/series/ResolvedSeriesData.hpp"
#include "ChartPlotter/series/XYSeries.hpp"
#include "ChartPlotter/strategy/ISeriesStrategy.hpp"

#include <QHash>
#include <QPointer>
#include <QVector>

namespace ChartPlotter {

struct ResolvedColumn {
  bool valid = false;
  QString errorMessage;

  QString name;
  int index = -1;
  ChartEnums::DataType type = ChartEnums::DataType::Unknown;
};

class SeriesDataResolver {
public:
  SeriesResolveResult
  resolve(const QVector<int> &xySeriesIndexes,
          const QVector<int> &pieSeriesIndexes,
          const QVector<QPointer<AbstractSeries>> &series,
          const QHash<DataSource *, int> &sourceIds,
          const QHash<int, DataSnapshot> &snapshots,
          QHash<PointCacheKey, PointCacheValue> *globalPointCache);

private:
  ResolvedSeriesData
  resolveXYSeries(int seriesIndex, QPointer<XYSeries> series,
                  const QHash<DataSource *, int> &sourceIds,
                  const QHash<int, DataSnapshot> &snapshots) const;

  ResolvedSeriesData
  resolvePieSeries(int seriesIndex, QPointer<PieSeries> series,
                   const QHash<DataSource *, int> &sourceIds,
                   const QHash<int, DataSnapshot> &snapshots) const;

  bool isSupportedXAxisType(ChartEnums::DataType type) const;
  bool isSupportedPieLabelType(ChartEnums::DataType type) const;

  bool checkSharedXYBinding(SeriesResolveResult &result) const;

  qint64 getColumnIndex(const DataSnapshot &snapshot,
                        const QString &colName) const;

  ResolvedColumn resolveColumn(const DataSnapshot &snapshot,
                               const ColumnBinding &binding,
                               const QString &role, int seriesIndex) const;
  void collectSharedXCategories(SeriesResolveResult &result,
                                const QHash<int, DataSnapshot> &snapshots);

  void calculateAbsoluteBounds(
      SeriesResolveResult &result, const QHash<int, DataSnapshot> &snapshots,
      QHash<PointCacheKey, PointCacheValue> *globalPointCache) const;
};

} // namespace ChartPlotter

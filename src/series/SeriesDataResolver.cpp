#include "ChartPlotter/series/SeriesDataResolver.hpp"
#include "ChartPlotter/types/DataRange.hpp"
#include "ChartPlotter/utils/DataRangeCalculator.hpp"

#include <algorithm>
#include <cmath>

namespace ChartPlotter {

SeriesResolveResult SeriesDataResolver::resolve(
    const QVector<int> &xySeriesIndexes, const QVector<int> &pieSeriesIndexes,
    const QVector<QPointer<AbstractSeries>> &series,
    const QHash<DataSource *, int> &sourceIds,
    const QHash<int, DataSnapshot> &snapshots,
    QHash<PointCacheKey, PointCacheValue> *globalPointCache) {
  SeriesResolveResult result;

  for (int seriesIndex : xySeriesIndexes) {
    if (seriesIndex < 0 || seriesIndex >= series.size()) {
      result.valid = false;
      result.errorMessage =
          QString("Invalid XY series index: %1").arg(seriesIndex);
      return result;
    }

    auto *xySeries = qobject_cast<XYSeries *>(series.at(seriesIndex));

    if (!xySeries) {
      result.valid = false;
      result.errorMessage =
          QString("Series %1 is not an XYSeries").arg(seriesIndex);
      return result;
    }

    // CP_DEBUG("Resolving series index {}", seriesIndex);
    ResolvedSeriesData resolved =
        resolveXYSeries(seriesIndex, xySeries, sourceIds, snapshots);

    if (!resolved.valid) {
      result.valid = false;
      result.errorMessage = resolved.errorMessage;
      return result;
    }

    result.xySeries.push_back(resolved);
  }

  for (int seriesIndex : pieSeriesIndexes) {
    if (seriesIndex < 0 || seriesIndex >= series.size()) {
      result.valid = false;
      result.errorMessage =
          QString("Invalid Pie series index: %1").arg(seriesIndex);
      return result;
    }

    auto *pieSeries = qobject_cast<PieSeries *>(series.at(seriesIndex));

    if (!pieSeries) {
      result.valid = false;
      result.errorMessage =
          QString("Series %1 is not a PieSeries").arg(seriesIndex);
      return result;
    }

    ResolvedSeriesData resolved =
        resolvePieSeries(seriesIndex, pieSeries, sourceIds, snapshots);

    if (!resolved.valid) {
      result.valid = false;
      result.errorMessage = resolved.errorMessage;
      return result;
    }

    result.pieSeries.push_back(resolved);
  }

  // CP_DEBUG("checking shared XY binding...");
  if (!checkSharedXYBinding(result)) {
    return result;
  }

  if (result.sharedXColumnType == ChartEnums::DataType::String) {
    // CP_DEBUG("collecting shared X categories...");
    collectSharedXCategories(result, snapshots);
  }

  // CP_DEBUG("calculating abs bounds...");
  calculateAbsoluteBounds(result, snapshots, globalPointCache);

  // CP_DEBUG("RESOLVED!!!!!!!!!!!");
  result.valid = true;
  return result;
}

ResolvedSeriesData SeriesDataResolver::resolveXYSeries(
    int seriesIndex, QPointer<XYSeries> series,
    const QHash<DataSource *, int> &sourceIds,
    const QHash<int, DataSnapshot> &snapshots) const {
  ResolvedSeriesData result;
  result.kind = ResolvedSeriesKind::XY;
  result.seriesIndex = seriesIndex;

  if (!series) {
    result.errorMessage = QString("Series %1 is null").arg(seriesIndex);
    return result;
  }

  DataSource *source = series->source();

  if (!source) {
    result.errorMessage =
        QString("XY series %1 has no DataSource").arg(seriesIndex);
    return result;
  }

  const int sourceId = sourceIds.value(source, -1);

  if (sourceId < 0) {
    result.errorMessage =
        QString("XY series %1 references an unknown DataSource")
            .arg(seriesIndex);
    return result;
  }

  // Special case: no data yet
  if (!snapshots.contains(sourceId)) {
    result.sourceId = sourceId;
    result.xColumnType = ChartEnums::DataType::Number;
    result.yColumnType = ChartEnums::DataType::Number;
    result.valid = true;
    return result;
  }

  const DataSnapshot &snapshot = snapshots[sourceId];

  ResolvedColumn xColumn =
      resolveColumn(snapshot, series->xBinding(), "x", seriesIndex);

  if (!xColumn.valid) {
    result.errorMessage = xColumn.errorMessage;
    return result;
  }

  ResolvedColumn yColumn =
      resolveColumn(snapshot, series->yBinding(), "y", seriesIndex);

  if (!yColumn.valid) {
    result.errorMessage = yColumn.errorMessage;
    return result;
  }

  if (!isSupportedXAxisType(xColumn.type)) {
    result.errorMessage =
        QString("XY series %1 x column '%2' has unsupported type")
            .arg(seriesIndex)
            .arg(xColumn.name);
    return result;
  }

  if (yColumn.type != ChartEnums::DataType::Number) {
    result.errorMessage = QString("XY series %1 y column '%2' must be Number")
                              .arg(seriesIndex)
                              .arg(yColumn.name);
    return result;
  }

  result.sourceId = sourceId;

  result.xColumnName = xColumn.name;
  result.yColumnName = yColumn.name;

  result.xColumnIndex = xColumn.index;
  result.yColumnIndex = yColumn.index;

  result.xColumnType = xColumn.type;
  result.yColumnType = yColumn.type;

  result.valid = true;
  return result;
}

ResolvedSeriesData SeriesDataResolver::resolvePieSeries(
    int seriesIndex, QPointer<PieSeries> series,
    const QHash<DataSource *, int> &sourceIds,
    const QHash<int, DataSnapshot> &snapshots) const {
  ResolvedSeriesData result;
  result.kind = ResolvedSeriesKind::Pie;
  result.seriesIndex = seriesIndex;

  if (!series) {
    result.errorMessage = QString("Series %1 is null").arg(seriesIndex);
    return result;
  }

  DataSource *source = series->source();

  if (!source) {
    result.errorMessage =
        QString("Pie series %1 has no DataSource").arg(seriesIndex);
    return result;
  }

  const int sourceId = sourceIds.value(source, -1);

  if (sourceId < 0) {
    result.errorMessage =
        QString("Pie series %1 references an unknown DataSource")
            .arg(seriesIndex);
    return result;
  }

  if (!snapshots.contains(sourceId)) {
    result.errorMessage =
        QString("Pie series %1 has no snapshot yet for sourceId %2")
            .arg(seriesIndex)
            .arg(sourceId);
    return result;
  }

  const DataSnapshot &snapshot = snapshots[sourceId];

  const QString labelName = series->label();
  const QString valueName = series->value();

  if (labelName.isEmpty()) {
    result.errorMessage =
        QString("Pie series %1 has empty label column").arg(seriesIndex);
    return result;
  }

  if (valueName.isEmpty()) {
    result.errorMessage =
        QString("Pie series %1 has empty value column").arg(seriesIndex);
    return result;
  }

  const int labelIndex = snapshot.columnIndex.contains(labelName)
                             ? snapshot.columnIndex.value(labelName)
                             : -1;
  const int valueIndex = snapshot.columnIndex.contains(valueName)
                             ? snapshot.columnIndex.value(valueName)
                             : -1;

  if (labelIndex < 0) {
    result.errorMessage = QString("Pie series %1 label column not found: '%2'")
                              .arg(seriesIndex)
                              .arg(labelName);
    return result;
  }

  if (valueIndex < 0) {
    result.errorMessage = QString("Pie series %1 value column not found: '%2'")
                              .arg(seriesIndex)
                              .arg(valueName);
    return result;
  }

  const ChartEnums::DataType labelType = snapshot.columnType(labelIndex);
  const ChartEnums::DataType valueType = snapshot.columnType(valueIndex);

  if (!isSupportedPieLabelType(labelType)) {
    result.errorMessage =
        QString("Pie series %1 label column '%2' has unsupported type")
            .arg(seriesIndex)
            .arg(labelName);
    return result;
  }

  if (valueType != ChartEnums::DataType::Number) {
    result.errorMessage =
        QString("Pie series %1 value column '%2' must be Number")
            .arg(seriesIndex)
            .arg(valueName);
    return result;
  }

  result.sourceId = sourceId;

  result.labelColumnName = labelName;
  result.valueColumnName = valueName;

  result.labelColumnIndex = labelIndex;
  result.valueColumnIndex = valueIndex;

  result.labelColumnType = labelType;
  result.valueColumnType = valueType;

  result.valid = true;
  return result;
}

bool SeriesDataResolver::checkSharedXYBinding(
    SeriesResolveResult &result) const {
  if (result.xySeries.isEmpty()) {
    return true;
  }

  const ResolvedSeriesData &first = result.xySeries.first();

  if (!first.valid) {
    result.valid = false;
    result.errorMessage = first.errorMessage;
    return false;
  }

  result.sharedXColumnName = first.xColumnName;
  result.sharedXColumnType = first.xColumnType;

  for (const ResolvedSeriesData &resolved : result.xySeries) {
    if (!resolved.valid) {
      result.valid = false;
      result.errorMessage = resolved.errorMessage;
      return false;
    }

    if (resolved.xColumnName != result.sharedXColumnName) {
      result.valid = false;
      result.errorMessage =
          QString("XY series %1 uses x column '%2', but shared x-axis expects "
                  "'%3'")
              .arg(resolved.seriesIndex)
              .arg(resolved.xColumnName)
              .arg(result.sharedXColumnName);
      return false;
    }

    if (resolved.xColumnType != result.sharedXColumnType) {
      result.valid = false;
      result.errorMessage =
          QString("XY series %1 x column '%2' has incompatible type")
              .arg(resolved.seriesIndex)
              .arg(resolved.xColumnName);
      return false;
    }

    if (resolved.yColumnType != ChartEnums::DataType::Number) {
      result.valid = false;
      result.errorMessage = QString("XY series %1 y column '%2' must be Number")
                                .arg(resolved.seriesIndex)
                                .arg(resolved.yColumnName);
      return false;
    }
  }

  return true;
}

bool SeriesDataResolver::isSupportedXAxisType(ChartEnums::DataType type) const {
  return type == ChartEnums::DataType::Number ||
         type == ChartEnums::DataType::Date ||
         type == ChartEnums::DataType::String;
}

bool SeriesDataResolver::isSupportedPieLabelType(
    ChartEnums::DataType type) const {
  return type == ChartEnums::DataType::String ||
         type == ChartEnums::DataType::Date ||
         type == ChartEnums::DataType::Number;
}

qint64 SeriesDataResolver::getColumnIndex(const DataSnapshot &snapshot,
                                          const QString &colName) const {
  return snapshot.columnIndex.contains(colName)
             ? snapshot.columnIndex.value(colName)
             : -1;
}

ResolvedColumn SeriesDataResolver::resolveColumn(const DataSnapshot &snapshot,
                                                 const ColumnBinding &binding,
                                                 const QString &role,
                                                 int seriesIndex) const {
  ResolvedColumn result;

  if (binding.kind == ColumnBindingKind::Invalid) {
    result.errorMessage = QString("Series %1 has invalid %2 column binding")
                              .arg(seriesIndex)
                              .arg(role);
    return result;
  }

  qint64 index = -1;

  if (binding.kind == ColumnBindingKind::Index) {
    index = binding.index;

    if (index < 0 || index >= snapshot.columnCount) {
      result.errorMessage =
          QString("Series %1 %2 column index %3 is out of range")
              .arg(seriesIndex)
              .arg(role)
              .arg(index);
      return result;
    }
  } else {
    index = getColumnIndex(snapshot, binding.name);

    if (index < 0) {
      result.errorMessage = QString("Series %1 %2 column not found: '%3'")
                                .arg(seriesIndex)
                                .arg(role)
                                .arg(binding.name);
      return result;
    }
  }

  result.index = index;
  result.name = snapshot.columnName(index);
  result.type = snapshot.columnType(index);
  result.valid = true;

  return result;
}

void SeriesDataResolver::collectSharedXCategories(
    SeriesResolveResult &result, const QHash<int, DataSnapshot> &snapshots) {
  for (ResolvedSeriesData &resolved : result.xySeries) {
    if (!resolved.valid) {
      continue;
    }
    const auto it = snapshots.constFind(resolved.sourceId);
    if (it == snapshots.constEnd()) {
      continue;
    }
    const DataSnapshot &snapshot = it.value();
    const qint64 col = resolved.xColumnIndex;
    if (col < 0 || col >= snapshot.columnCount) {
      continue;
    }

    const auto &localCategories = snapshot.columns.at(col).categories;
    resolved.localToGlobalXMap.resize(localCategories.size());

    for (qsizetype i = 0; i < localCategories.size(); ++i) {
      int globalId = result.sharedXCategories.intern(localCategories[i]);
      resolved.localToGlobalXMap[i] = static_cast<double>(globalId);
    }
  }
}

void SeriesDataResolver::calculateAbsoluteBounds(
    SeriesResolveResult &result, const QHash<int, DataSnapshot> &snapshots,
    QHash<PointCacheKey, PointCacheValue> *globalPointCache) const {

  result.absoluteXRange = {};
  result.absoluteYRange = {};

  // Prevents recalculating the same delta if multiple series share identical
  // data
  QSet<PointCacheKey> boundsProcessedThisFrame;

  for (ResolvedSeriesData &resolved : result.xySeries) {
    if (!resolved.valid) {
      continue;
    }

    const auto it = snapshots.constFind(resolved.sourceId);
    if (it == snapshots.constEnd()) {
      continue;
    }

    const DataSnapshot &snapshot = it.value();
    PointCacheKey key{resolved.sourceId, resolved.xColumnIndex,
                      resolved.yColumnIndex};
    PointCacheValue &cache = (*globalPointCache)[key];

    if (cache.epochId != snapshot.epochId) {
      cache.epochId = snapshot.epochId;
      cache.processedRowCount = 0;
      cache.points.clear();
      cache.resolvedXRange = {};
      cache.resolvedYRange = {};
    }

    // `processedRowCount` is an ABSOLUTE, monotonic high-water row id (the same
    // mark the rendering strategy maintains), not a live count. Convert it into
    // a live index against the current window [firstRowId, firstRowId +
    // rowCount) so we only scan the newly appended tail rows. When nothing has
    // been evicted firstRowId == 0 and this collapses to the old `fromRow ==
    // processedRowCount` behaviour.
    const quint64 liveStart = static_cast<quint64>(snapshot.firstRowId);
    const quint64 liveEnd = liveStart + static_cast<quint64>(snapshot.rowCount);
    const quint64 absFrom = std::max(cache.processedRowCount, liveStart);

    if (absFrom < liveEnd && !boundsProcessedThisFrame.contains(key)) {
      const quint64 liveFrom = absFrom - liveStart;

      DataRange deltaX;
      DataRange deltaY;

      // X Bounds
      if (resolved.xColumnType == ChartEnums::DataType::String) {
        double maxId = static_cast<double>(
            std::max(0, result.sharedXCategories.size() - 1));
        deltaX = DataRange{.min = 0.0, .max = maxId, .valid = true};
      } else {
        deltaX = DataRangeCalculator::calculateColumnRange(
            snapshot, resolved.xColumnIndex, liveFrom);
      }

      // Y Bounds
      deltaY = DataRangeCalculator::calculateColumnRange(
          snapshot, resolved.yColumnIndex, liveFrom);

      cache.resolvedXRange =
          DataRangeCalculator::unionRange(cache.resolvedXRange, deltaX);
      cache.resolvedYRange =
          DataRangeCalculator::unionRange(cache.resolvedYRange, deltaY);

      boundsProcessedThisFrame.insert(key);

      // CRITICAL: Do NOT update cache.processedRowCount here!
      // The rendering strategies still need to read these exact rows to extract
      // QPointFs.
    }

    // Sliding-window front eviction. The incrementally-unioned X range can only
    // grow, so once old rows are dropped (firstRowId > 0) its `min` still
    // carries the evicted prefix. Left unhandled this freezes the auto-scaled
    // viewport at the full history while the live points slide off to the right
    // and vanish — exactly the failure seen when switching maxCacheRows from
    // unlimited to limited mid-stream. x is non-decreasing in the streaming/cap
    // case (the same assumption the strategy's eviction and slicing rely on),
    // so the live lower edge is the first live row's x; raise `min` to it so
    // the absolute range tracks the live window.
    if (snapshot.firstRowId > 0 && cache.resolvedXRange.valid &&
        resolved.xColumnType != ChartEnums::DataType::String) {
      for (qint64 row = 0; row < snapshot.rowCount; ++row) {
        const double v = snapshot.valueAt(resolved.xColumnIndex, row);
        if (!std::isnan(v) && std::isfinite(v)) {
          cache.resolvedXRange.min = std::max(cache.resolvedXRange.min, v);
          break;
        }
      }
    }

    resolved.absoluteXRange = cache.resolvedXRange;
    resolved.absoluteYRange = cache.resolvedYRange;

    result.absoluteXRange = DataRangeCalculator::unionRange(
        result.absoluteXRange, resolved.absoluteXRange);
    result.absoluteYRange = DataRangeCalculator::unionRange(
        result.absoluteYRange, resolved.absoluteYRange);
  }
}

} // namespace ChartPlotter

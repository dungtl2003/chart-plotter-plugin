#include "ChartPlotter/series/SeriesDataResolver.hpp"

namespace ChartPlotter {

SeriesResolveResult
SeriesDataResolver::resolve(const QVector<int> &xySeriesIndexes,
                            const QVector<int> &pieSeriesIndexes,
                            const QVector<QPointer<AbstractSeries>> &series,
                            const QHash<DataSource *, int> &sourceIds,
                            const QHash<int, DataSnapshot> &snapshots) const {
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

  if (!checkSharedXYBinding(result)) {
    return result;
  }

  if (result.sharedXColumnType == ChartEnums::DataType::String) {
    collectSharedXCategories(result, snapshots);
  }

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

  if (!snapshots.contains(sourceId)) {
    result.errorMessage =
        QString("XY series %1 has no snapshot yet for sourceId %2")
            .arg(seriesIndex)
            .arg(sourceId);
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

int SeriesDataResolver::getColumnIndex(const DataSnapshot &snapshot,
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

  int index = -1;

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
    SeriesResolveResult &result,
    const QHash<int, DataSnapshot> &snapshots) const {
  for (const ResolvedSeriesData &resolved : result.xySeries) {
    if (!resolved.valid) {
      continue;
    }
    const auto it = snapshots.constFind(resolved.sourceId);
    if (it == snapshots.constEnd()) {
      continue;
    }
    const DataSnapshot &snapshot = it.value();
    const int col = resolved.xColumnIndex;
    if (col < 0 || col >= snapshot.columnCount) {
      continue;
    }
    for (int row = 0; row < snapshot.rowCount; ++row) {
      const QVariant v = snapshot.valueAt(col, row);
      if (!v.isValid() || v.isNull()) {
        continue;
      }
      result.sharedXCategories.intern(v.toString());
    }
  }
}

} // namespace ChartPlotter

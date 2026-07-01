#include "ChartPlotter/strategy/LineSeriesStrategy.hpp"

#include "ChartConstants.hpp"
#include "ChartPlotter/constants/ChartConstants.hpp"
#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/downsample/LargestTriangleThreeBuckets.hpp"
#include "ChartPlotter/downsample/StationaryMinMaxDownsampler.hpp"
#include "ChartPlotter/series/LineSeries.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QPointF>
#include <QVector>

#include <algorithm>
#include <deque>
#include <iterator>
#include <memory>

namespace ChartPlotter {

std::unique_ptr<RenderData> LineSeriesStrategy::build(
    const AbstractSeries &series, const ResolvedSeriesData &resolved,
    const DataSnapshot &snapshot, SeriesBuildContext &context) {
  auto data = std::make_unique<LineRenderData>();

  auto result = validateTyAndColIndex(resolved, snapshot);
  if (!result) {
    CP_WARN(result.error().toStdString());
    return data;
  }

  result = loadSeriesConfig(data.get(), series, context);
  if (!result) {
    CP_WARN(result.error().toStdString());
    return data;
  }

  PointCacheKey key{resolved.sourceId, resolved.xColumnIndex,
                    resolved.yColumnIndex};
  PointCacheValue &cache = (*context.globalPointCache)[key];

  if (cache.epochId != snapshot.epochId) {
    cache.epochId = snapshot.epochId;
    cache.processedRowCount = 0;
    cache.points.clear();
    cache.lodPyramid.clear();
  }

  // `processedRowCount` is an ABSOLUTE, monotonic row id (high-water mark), not a
  // live count. The buffer's live window is the absolute id range
  // [firstRowId, firstRowId + rowCount); front eviction advances firstRowId but
  // never rewinds the high-water, so the tail-append logic is untouched by it.
  const quint64 liveStart = static_cast<quint64>(snapshot.firstRowId);
  const quint64 liveEnd = liveStart + static_cast<quint64>(snapshot.rowCount);

  // Did the cache change as a pure tail append (the streaming common case)? If
  // so the LOD pyramid can extend incrementally instead of rebuilding.
  bool appendedOnly = false;

  const quint64 absFrom = std::max(cache.processedRowCount, liveStart);
  if (absFrom < liveEnd) {
    // CP_DEBUG("Processing new delta rows...");
    const qsizetype liveFrom = static_cast<qsizetype>(absFrom - liveStart);
    const qsizetype liveTo = snapshot.rowCount;

    QVector<QPointF> newPoints;
    newPoints.reserve(liveTo - liveFrom);

    auto convertResult = convertRowsToPoints(newPoints, series, resolved,
                                             snapshot, context, liveFrom, liveTo);
    if (!convertResult) {
      CP_WARN(convertResult.error().toStdString());
      return data;
    }

    const bool wasEmpty = cache.points.empty();
    const double prevMaxX = wasEmpty ? 0.0 : cache.points.back().x();
    const double deltaMinX =
        newPoints.isEmpty() ? prevMaxX : newPoints.first().x();
    // Tail append if we already had data and every new point sits at/after it.
    // (The active path always appends; this also stays correct if the merge
    // branch ever inserts out-of-order data — then we fall back to a rebuild.)
    appendedOnly = !wasEmpty && deltaMinX >= prevMaxX;

    if (data->dataIsSortedByX) {
      // Sort only the new delta points
      std::sort(
          newPoints.begin(), newPoints.end(),
          [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });

      // Merge sort the newly sorted delta with the already sorted cache
      if (cache.points.empty()) {
        cache.points.assign(newPoints.cbegin(), newPoints.cend());
      } else {
        std::deque<QPointF> merged;
        std::merge(
            cache.points.begin(), cache.points.end(), newPoints.begin(),
            newPoints.end(), std::back_inserter(merged),
            [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });

        cache.points = std::move(merged);
      }
    } else {
      // No sorting required, simply append the new points
      cache.points.insert(cache.points.end(), newPoints.cbegin(),
                          newPoints.cend());
    }

    cache.processedRowCount = liveEnd;
  }

  // Sliding-window front eviction. Once the buffer has dropped old rows
  // (firstRowId > 0), cached points that now fall before the live window are
  // stale. x is assumed non-decreasing with row order (the streaming case this
  // cap targets, and the same assumption the slicing binary-search relies on),
  // so the stale points are a contiguous prefix located via lower_bound.
  //
  // Both halves of the eviction are now O(evicted), not O(live window): the
  // point deque frees whole front nodes without shifting the tail, and
  // LodPyramid::trimFront drops aligned front buckets in place (no rebuild). We
  // ask the pyramid how many points it can drop while staying bucket-aligned and
  // erase exactly that many points so the two stay in sync; at most one
  // coarsest-bucket span of stale points lingers as harmless slack to the left
  // of the viewport. This keeps a capped bulk load smooth and O(totalRows) with
  // no periodic compaction spike.
  if (snapshot.firstRowId > 0 && !cache.points.empty()) {
    double firstLiveX = 0.0;
    if (firstLiveXValue(snapshot, resolved, firstLiveX)) {
      const auto cut = std::lower_bound(
          cache.points.begin(), cache.points.end(), firstLiveX,
          [](const QPointF &p, double v) { return p.x() < v; });
      const qsizetype deadCount =
          static_cast<qsizetype>(cut - cache.points.begin());
      if (deadCount > 0) {
        const qsizetype dropped = cache.lodPyramid.trimFront(deadCount);
        if (dropped > 0) {
          cache.points.erase(cache.points.begin(),
                             cache.points.begin() + dropped);
        }
      }
    }
  }

  // Keep the LOD pyramid in sync. O(delta) on a tail append (and trimFront above
  // already kept the front aligned, so an eviction does NOT force a rebuild),
  // O(N) only on a true full rebuild (first load / out-of-order data), and an
  // instant no-op when nothing changed — essentially free on plain zoom/pan.
  cache.lodPyramid.update(cache.points, appendedOnly);

  // Slice the cached (full) series to the visible X range. calculateBound
  // binary-searches the cache (O(log N)); we then read only the pyramid buckets
  // overlapping that range (O(visible buckets) ≈ O(pixels)) instead of scanning
  // every visible point. The big per-event copy/scan that made pan/zoom stutter
  // on millions of points is gone.
  if (!cache.points.empty()) {
    const auto bound = calculateBound(cache.points, context);
    const qsizetype loIdx =
        static_cast<qsizetype>(bound.first - cache.points.cbegin());
    const qsizetype hiIdx =
        static_cast<qsizetype>(bound.second - cache.points.cbegin());
    const qsizetype visibleCount = hiIdx - loIdx;

    const bool needDownsample = context.preferredTotalPoints > 0 &&
                                visibleCount > context.preferredTotalPoints;

    if (!needDownsample) {
      // Few enough visible points — draw them as-is (only this small slice is
      // copied).
      data->points = QVector<QPointF>(bound.first, bound.second);
    } else if (!cache.lodPyramid.isEmpty()) {
      // Query the LOD pyramid: O(visible buckets) ≈ O(pixels).
      if (context.downsampleMode == ChartEnums::DownsampleMode::Lttb) {
        data->points = cache.lodPyramid.queryLttb(loIdx, hiIdx,
                                                  context.preferredTotalPoints);
      } else {
        // min/max emits up to 2 points per bucket, so target half as many
        // buckets to keep the output near the requested point budget.
        const qsizetype maxBuckets =
            std::max<qsizetype>(1, context.preferredTotalPoints / 2);
        data->points = cache.lodPyramid.queryMinMax(loIdx, hiIdx, maxBuckets);
      }
    } else {
      // Fallback (no pyramid yet, the first frames of a load). The downsamplers
      // take contiguous QVector iterators, so materialize the visible slice once
      // (the deque's nodes aren't contiguous). This path is transient.
      const QVector<QPointF> slice(bound.first, bound.second);
      if (context.downsampleMode == ChartEnums::DownsampleMode::Lttb) {
        // LTTB mode: reduce the visible slice straight through LTTB so the toggle
        // still switches algorithm here.
        data->points.resize(context.preferredTotalPoints);
        LargestTriangleThreeBuckets lttb;
        lttb.downsample(slice.cbegin(), slice.size(), data->points.begin(),
                        context.preferredTotalPoints);
      } else {
        // min/max mode: stationary min/max over the visible range, anchored to a
        // data-space grid so it stays shimmer-free.
        const bool hasView = context.viewportXRange.valid;
        const double vmin =
            hasView ? context.viewportXRange.min : slice.first().x();
        const double vmax =
            hasView ? context.viewportXRange.max : slice.last().x();
        const qsizetype numBuckets =
            std::max<qsizetype>(1, context.preferredTotalPoints / 2);
        const double gridWidth =
            (vmax - vmin) / static_cast<double>(numBuckets);
        if (gridWidth > 0.0) {
          StationaryMinMaxDownsampler downsampler;
          data->points =
              downsampler.downsample(slice.cbegin(), slice.cend(), gridWidth);
        } else {
          data->points = slice;
        }
      }
    }
  }

  // CP_DEBUG("calculating data range...");
  for (const auto &p : data->points) {
    DataRange::includeValue(data->xRange, p.x());
    DataRange::includeValue(data->yRange, p.y());
  }

  // CP_DEBUG("finished!!!!!!");
  data->valid = true;
  return data;
}

std::expected<void, QString> LineSeriesStrategy::convertRowsToPoints(
    QVector<QPointF> &destination, const AbstractSeries &series,
    const ResolvedSeriesData &resolved, const DataSnapshot &snapshot,
    const SeriesBuildContext &context, qsizetype fromRow,
    qsizetype endRow) const {
  const int xIndex = resolved.xColumnIndex;
  const int yIndex = resolved.yColumnIndex;
  const ChartEnums::DataType xType = resolved.xColumnType;

  if (xType != ChartEnums::DataType::Number &&
      xType != ChartEnums::DataType::Date &&
      xType != ChartEnums::DataType::String) {
    return std::unexpected(
        "LineSeriesStrategy::build: unsupported x column type");
  }

  for (qsizetype row = fromRow; row < endRow; ++row) {
    double x = snapshot.valueAt(xIndex, row);
    double y = snapshot.valueAt(yIndex, row);

    if (std::isnan(x) || std::isnan(y)) {
      continue;
    }

    if (xType == ChartEnums::DataType::String) {
      int localId = static_cast<int>(x);

      if (localId < 0 || localId >= resolved.localToGlobalXMap.size()) {
        continue;
      }

      x = resolved.localToGlobalXMap[localId];
    }

    destination.append(QPointF(x, y));
  }

  return {};
}

bool LineSeriesStrategy::firstLiveXValue(const DataSnapshot &snapshot,
                                         const ResolvedSeriesData &resolved,
                                         double &out) const {
  const int xIndex = resolved.xColumnIndex;
  const ChartEnums::DataType xType = resolved.xColumnType;

  // The earliest live row's x is the lower edge of the current window. Skip
  // leading NaN x (rows that produced no point anyway).
  for (qint64 row = 0; row < snapshot.rowCount; ++row) {
    double x = snapshot.valueAt(xIndex, row);
    if (std::isnan(x)) {
      continue;
    }
    if (xType == ChartEnums::DataType::String) {
      const int localId = static_cast<int>(x);
      if (localId < 0 || localId >= resolved.localToGlobalXMap.size()) {
        continue;
      }
      x = resolved.localToGlobalXMap[localId];
    }
    out = x;
    return true;
  }
  return false;
}

std::expected<void, QString>
LineSeriesStrategy::loadSeriesConfig(LineRenderData *data,
                                     const AbstractSeries &series,
                                     const SeriesBuildContext &context) const {
  assert(data != nullptr);

  const auto *lineSeries = qobject_cast<const LineSeries *>(&series);

  if (!lineSeries) {
    return std::unexpected(
        "LineSeriesStrategy::build: series is not LineSeries");
  }

  const float width =
      std::clamp(lineSeries->useGlobalStrokeWidth() ? context.globalLineWidth
                                                    : lineSeries->strokeWidth(),
                 ChartConstants::LINE_STROKE_WIDTH_MIN,
                 ChartConstants::LINE_STROKE_WIDTH_MAX);
  const float aa =
      std::clamp(lineSeries->useGlobalAntialias() ? context.globalAntialiasing
                                                  : lineSeries->antialias(),
                 ChartConstants::LINE_AA_MIN, ChartConstants::LINE_AA_MAX);

  data->marker.color = lineSeries->markerColor();
  data->marker.visible = lineSeries->markerVisible();
  data->stroke.color = lineSeries->strokeColor();
  data->stroke.width = width;
  data->stroke.pattern = lineSeries->strokePattern();
  data->stroke.dashStyle = DashStyle{
      .length = std::max(15.0f, width * 3.0f),
      .gap = std::max(10.0f, width * 2.0f),
  };
  data->stroke.dotStyle = DotStyle{
      .gap = std::max(10.0f, width),
  };
  data->antialias = aa;

  return {};
}

std::pair<std::deque<QPointF>::const_iterator,
          std::deque<QPointF>::const_iterator>
LineSeriesStrategy::calculateBound(const std::deque<QPointF> &points,
                                   const SeriesBuildContext &context) const {
  const double viewMin = context.viewportXRange.min;
  const double viewMax = context.viewportXRange.max;

  // Find the first point that is >= viewMin
  auto leftIt = std::lower_bound(
      points.begin(), points.end(), viewMin,
      [](const QPointF &p, double val) { return p.x() < val; });
  if (leftIt != points.begin()) {
    --leftIt;
  }

  auto rightIt = std::upper_bound(
      leftIt, points.end(), viewMax,
      [](double val, const QPointF &p) { return val < p.x(); });
  if (rightIt != points.end()) {
    ++rightIt;
  }

  return {leftIt, rightIt};
}

std::expected<void, QString>
LineSeriesStrategy::validateTyAndColIndex(const ResolvedSeriesData &resolved,
                                          const DataSnapshot &snapshot) const {
  const int xIndex = resolved.xColumnIndex;
  const int yIndex = resolved.yColumnIndex;
  const ChartEnums::DataType xType = resolved.xColumnType;
  const ChartEnums::DataType yType = resolved.yColumnType;

  if (!resolved.valid) {
    return std::unexpected(
        QString("LineSeriesStrategy::build: resolved series is invalid: %1")
            .arg(resolved.errorMessage));
  }

  if (xIndex < 0 || xIndex >= snapshot.columnCount) {
    return std::unexpected(
        QString("LineSeriesStrategy::build: invalid x column index %1")
            .arg(xIndex));
  }

  if (yIndex < 0 || yIndex >= snapshot.columnCount) {
    return std::unexpected(
        QString("LineSeriesStrategy::build: invalid y column index %1")
            .arg(yIndex));
  }

  if (yType != ChartEnums::DataType::Number) {
    return std::unexpected(
        "LineSeriesStrategy::build: y column must be Number");
  }

  return {};
}

} // namespace ChartPlotter

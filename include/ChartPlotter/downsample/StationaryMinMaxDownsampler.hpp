#pragma once

#include <QPointF>
#include <QVector>

namespace ChartPlotter {

// Stationary (shimmer-free) min/max downsampler.
//
// The classic instability of LTTB / count-proportional bucketing is that the
// bucket boundaries are derived from the *total* number of points, so every new
// streamed point shifts every boundary and re-selects every representative —
// the whole chart crawls.
//
// Here the buckets live on a FIXED data-space grid whose lines are at integer
// multiples of `gridWidth` (a global grid, independent of how many points exist
// or where the window currently sits). A point's bucket is `floor(x/gridWidth)`.
// For a fixed zoom level the grid never moves, so appending a point only changes
// the single bucket it falls into; all other buckets keep the exact same min/max
// and therefore the same output. History stops moving.
//
// Each non-empty bucket contributes its min and max Y at the bucket-center X
// (the true vertical envelope, so spikes are preserved).
class StationaryMinMaxDownsampler {
public:
  using ConstIterator = QVector<QPointF>::const_iterator;

  // Reads the half-open range [first, last) directly (no copy of the slice) and
  // returns the per-bucket min/max envelope points. The range must be ascending
  // in x (the line cache already is); a single pass groups contiguous runs that
  // share a bucket. `gridWidth` is the data-space width of one bucket (> 0).
  //
  // Large ranges are reduced in parallel: each thread aggregates a contiguous
  // chunk into bucket min/max, then the partials are merged (only the bucket on
  // a chunk boundary needs combining).
  QVector<QPointF> downsample(ConstIterator first, ConstIterator last,
                              double gridWidth) const;
};

} // namespace ChartPlotter

#pragma once

#include <QPointF>
#include <QVector>

#include <deque>

namespace ChartPlotter {

// Level-of-detail (LOD) pyramid over an x-sorted point series. Stores min/max
// aggregates per bucket and serves two query modes from the same structure:
// queryMinMax() (min/max envelope) and queryLttb() (oversampled candidates fed
// through LTTB for a clean single line) — hence the neutral name.
//
// The problem it solves: re-scanning every visible point on each zoom/pan is
// O(visible N) — millions of points per interaction when zoomed out. The
// pyramid precomputes min/max aggregates at geometrically coarser resolutions
// (each level groups `kBranch` buckets of the level below), so at render time
// we read only the buckets at the level whose resolution matches the screen.
// Per-query cost becomes O(visible buckets) ≈ O(pixels), independent of N.
//
// Buckets are index-based (a bucket spans a fixed *count* of points), which
// makes the prefix stable under tail appends — streaming a new point only
// rewrites the last partial bucket of each level, so history never re-selects.
// The query maps a visible x-range to an index range via the caller's binary
// search (the series is assumed x-sorted, the same assumption the existing
// slicing already relies on).
class LodPyramid {
public:
  void clear();

  // Make the pyramid reflect `points` (x-sorted). `appendedOnly` means the
  // change versus last time was purely new points at the tail (the common
  // streaming case) — then only the tail is rebuilt; otherwise it is rebuilt
  // from scratch. A no-op when the point count is unchanged.
  void update(const std::deque<QPointF> &points, bool appendedOnly);

  // Drop the oldest `maxPoints` points from the front WITHOUT rebuilding. Only
  // whole buckets can be discarded while keeping the survivors aligned to the
  // new index 0, so the amount actually removed is rounded down to a multiple
  // of the coarsest level's span (which divides every finer span); each level
  // just pops that many front buckets. Returns the number of points actually
  // dropped — the caller MUST drop exactly that many from the front of the
  // point series to keep the pyramid's indices in sync. Returns 0 (no-op) when
  // the pyramid is empty or fewer than one coarsest-span points are stale.
  qsizetype trimFront(qsizetype maxPoints);

  bool isEmpty() const { return m_levels.isEmpty(); }

  // Min/max envelope for the visible raw index range [loIdx, hiIdx), targeting
  // ~maxBuckets buckets. Picks the finest level that keeps the bucket count
  // within ~kBranch×maxBuckets. Returns up to two points (the actual min-Y and
  // max-Y samples, at their true x) per bucket.
  QVector<QPointF> queryMinMax(qsizetype loIdx, qsizetype hiIdx,
                               qsizetype maxBuckets) const;

  // Clean single line for the visible range, ~targetPoints points. Reads a
  // finer (oversampled) level for candidate points, then runs LTTB over them so
  // the result follows the real trajectory instead of a min/max band.
  QVector<QPointF> queryLttb(qsizetype loIdx, qsizetype hiIdx,
                             qsizetype targetPoints) const;

private:
  // The actual min-Y and max-Y samples of a bucket (real coordinates), so both
  // the envelope and the LTTB candidates sit on true data points.
  struct Bucket {
    QPointF lo; // sample with the smallest y
    QPointF hi; // sample with the largest y
  };
  struct Level {
    qsizetype span = 0; // raw points covered by one bucket at this level
    // deque so trimFront() can pop stale front buckets in O(removed) instead of
    // memmoving the whole bucket array down on every sliding-window eviction.
    std::deque<Bucket> buckets;
  };

  // (Re)build level 0 from raw index `fromRaw` and cascade the change upward,
  // adding coarser levels until the top is small enough.
  void rebuildFrom(const std::deque<QPointF> &points, qsizetype fromRaw);

  // Finest level whose bucket span does not exceed `spanNeeded` (>= level 0).
  qsizetype pickLevel(qsizetype spanNeeded) const;

  QVector<Level> m_levels; // ascending span: m_levels[0] is the finest
  qsizetype m_builtCount = 0;

  static constexpr qsizetype kBranch = 4;          // children per bucket
  static constexpr qsizetype kMinTopBuckets = 128; // stop coarsening below this
};

} // namespace ChartPlotter

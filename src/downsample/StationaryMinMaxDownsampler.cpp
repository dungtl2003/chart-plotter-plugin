#include "ChartPlotter/downsample/StationaryMinMaxDownsampler.hpp"

#include <QFuture>
#include <QThread>
#include <QtConcurrent>

#include <algorithm>
#include <cmath>

namespace ChartPlotter {

namespace {

// One bucket's aggregate: its grid index and the min/max Y seen in it.
struct BucketAgg {
  qint64 bucket;
  double minY;
  double maxY;
};

// Single pass over [lo, hi): because x is ascending, bucket indices are
// monotonic, so the result is already sorted and compact (one entry per
// contiguous run that shares a bucket).
QVector<BucketAgg>
reduceChunk(QVector<QPointF>::const_iterator first, qsizetype lo, qsizetype hi,
            double gridWidth) {
  QVector<BucketAgg> res;

  bool have = false;
  qint64 cur = 0;
  double minY = 0.0;
  double maxY = 0.0;

  for (qsizetype i = lo; i < hi; ++i) {
    const QPointF &p = *(first + i);
    const qint64 b = static_cast<qint64>(std::floor(p.x() / gridWidth));

    if (!have) {
      cur = b;
      minY = maxY = p.y();
      have = true;
    } else if (b == cur) {
      minY = std::min(minY, p.y());
      maxY = std::max(maxY, p.y());
    } else {
      res.push_back({cur, minY, maxY});
      cur = b;
      minY = maxY = p.y();
    }
  }

  if (have) {
    res.push_back({cur, minY, maxY});
  }

  return res;
}

} // namespace

QVector<QPointF>
StationaryMinMaxDownsampler::downsample(ConstIterator first, ConstIterator last,
                                        double gridWidth) const {
  const qsizetype n = last - first;
  if (n <= 0 || gridWidth <= 0.0) {
    return {};
  }

  int threadCount = QThread::idealThreadCount();
  if (threadCount < 1) {
    threadCount = 1;
  }

  // Threading overhead isn't worth it for small ranges.
  constexpr qsizetype kParallelThreshold = 100000;
  if (n < kParallelThreshold) {
    threadCount = 1;
  }

  QVector<QVector<BucketAgg>> partials;

  if (threadCount == 1) {
    partials.append(reduceChunk(first, 0, n, gridWidth));
  } else {
    const qsizetype perThread = n / threadCount;
    QVector<QFuture<QVector<BucketAgg>>> futures;
    futures.reserve(threadCount);
    for (int t = 0; t < threadCount; ++t) {
      const qsizetype lo = t * perThread;
      const qsizetype hi = (t == threadCount - 1) ? n : lo + perThread;
      futures.append(QtConcurrent::run([first, lo, hi, gridWidth]() {
        return reduceChunk(first, lo, hi, gridWidth);
      }));
    }
    for (int t = 0; t < threadCount; ++t) {
      partials.append(futures[t].result()); // blocks until chunk t is done
    }
  }

  // Concatenate the (already-sorted) partials in order; chunk t's buckets are
  // all <= chunk t+1's, so only a shared boundary bucket needs combining.
  QVector<BucketAgg> merged;
  for (const QVector<BucketAgg> &part : partials) {
    for (const BucketAgg &a : part) {
      if (!merged.isEmpty() && merged.back().bucket == a.bucket) {
        merged.back().minY = std::min(merged.back().minY, a.minY);
        merged.back().maxY = std::max(merged.back().maxY, a.maxY);
      } else {
        merged.push_back(a);
      }
    }
  }

  // Emit min/max at the (fixed) bucket center.
  QVector<QPointF> out;
  out.reserve(merged.size() * 2);
  for (const BucketAgg &a : merged) {
    const double cx = (static_cast<double>(a.bucket) + 0.5) * gridWidth;
    if (a.minY == a.maxY) {
      out.append(QPointF(cx, a.minY));
    } else {
      out.append(QPointF(cx, a.minY));
      out.append(QPointF(cx, a.maxY));
    }
  }

  return out;
}

} // namespace ChartPlotter

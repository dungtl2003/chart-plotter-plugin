#include "ChartPlotter/downsample/LodPyramid.hpp"

#include "ChartPlotter/downsample/LargestTriangleThreeBuckets.hpp"

#include <algorithm>

namespace ChartPlotter {

void LodPyramid::clear() {
  m_levels.clear();
  m_builtCount = 0;
}

void LodPyramid::update(const std::deque<QPointF> &points, bool appendedOnly) {
  const qsizetype n = points.size();

  // Unchanged since last build — nothing to do (this is the hot path on every
  // zoom/pan frame where no new data arrived).
  if (n == m_builtCount && !m_levels.isEmpty()) {
    return;
  }

  if (n == 0) {
    clear();
    return;
  }

  if (!appendedOnly || m_levels.isEmpty() || n < m_builtCount) {
    m_levels.clear();
    rebuildFrom(points, 0);
  } else {
    // Pure tail append: only the buckets covering [m_builtCount, n) change.
    rebuildFrom(points, m_builtCount);
  }

  m_builtCount = n;
}

void LodPyramid::rebuildFrom(const std::deque<QPointF> &points,
                             qsizetype fromRaw) {
  const qsizetype n = static_cast<qsizetype>(points.size());

  // ---- Level 0: aggregate raw points, span == kBranch ----
  if (m_levels.isEmpty()) {
    m_levels.append(Level{kBranch, {}});
  }
  {
    Level &l0 = m_levels[0];
    const qsizetype span = l0.span;
    qsizetype startB = fromRaw / span;
    startB = std::min(startB, static_cast<qsizetype>(l0.buckets.size()));
    l0.buckets.resize(startB); // drop the (partial) tail we are about to redo

    for (qsizetype b = startB; b * span < n; ++b) {
      const qsizetype s = b * span;
      const qsizetype e = std::min(n, s + span);
      QPointF lo = points[s];
      QPointF hi = points[s];
      for (qsizetype i = s + 1; i < e; ++i) {
        const QPointF &p = points[i];
        if (p.y() < lo.y()) {
          lo = p;
        }
        if (p.y() > hi.y()) {
          hi = p;
        }
      }
      l0.buckets.push_back(Bucket{lo, hi});
    }
  }

  // ---- Existing coarser levels: cascade the rebuilt tail upward ----
  qsizetype prevStart = fromRaw / m_levels[0].span;
  for (qsizetype k = 1; k < m_levels.size(); ++k) {
    Level &lk = m_levels[k];
    const Level &lp = m_levels[k - 1];
    const qsizetype lpCount = static_cast<qsizetype>(lp.buckets.size());

    qsizetype startB = prevStart / kBranch;
    startB = std::min(startB, static_cast<qsizetype>(lk.buckets.size()));
    lk.buckets.resize(startB);

    for (qsizetype b = startB; b * kBranch < lpCount; ++b) {
      const qsizetype s = b * kBranch;
      const qsizetype e = std::min(lpCount, s + kBranch);
      QPointF lo = lp.buckets[s].lo;
      QPointF hi = lp.buckets[s].hi;
      for (qsizetype i = s + 1; i < e; ++i) {
        if (lp.buckets[i].lo.y() < lo.y()) {
          lo = lp.buckets[i].lo;
        }
        if (lp.buckets[i].hi.y() > hi.y()) {
          hi = lp.buckets[i].hi;
        }
      }
      lk.buckets.push_back(Bucket{lo, hi});
    }
    prevStart = startB;
  }

  // ---- Add coarser levels until the top is small enough ----
  while (static_cast<qsizetype>(m_levels.last().buckets.size()) >
         kMinTopBuckets) {
    const Level &lp = m_levels.last();
    const qsizetype lpCount = static_cast<qsizetype>(lp.buckets.size());
    Level lk{lp.span * kBranch, {}};
    for (qsizetype b = 0; b * kBranch < lpCount; ++b) {
      const qsizetype s = b * kBranch;
      const qsizetype e = std::min(lpCount, s + kBranch);
      QPointF lo = lp.buckets[s].lo;
      QPointF hi = lp.buckets[s].hi;
      for (qsizetype i = s + 1; i < e; ++i) {
        if (lp.buckets[i].lo.y() < lo.y()) {
          lo = lp.buckets[i].lo;
        }
        if (lp.buckets[i].hi.y() > hi.y()) {
          hi = lp.buckets[i].hi;
        }
      }
      lk.buckets.push_back(Bucket{lo, hi});
    }
    m_levels.append(std::move(lk));
  }
}

qsizetype LodPyramid::trimFront(qsizetype maxPoints) {
  if (m_levels.isEmpty() || maxPoints <= 0) {
    return 0;
  }

  // Round down to a whole number of coarsest-level buckets. The top span is a
  // multiple of every finer level's span, so dropping `drop` points means
  // dropping exactly `drop / span` front buckets at each level, and every
  // survivor keeps summarizing the same (now re-indexed) points — no recompute.
  const qsizetype topSpan = m_levels.last().span;
  // Never drop more than the pyramid actually covers (guards the documented
  // x-non-decreasing assumption: if a caller ever asks to trim past the built
  // range, clamp to a whole number of top buckets instead of over-erasing).
  const qsizetype clamped = std::min(maxPoints, m_builtCount);
  const qsizetype drop = (clamped / topSpan) * topSpan;
  if (drop <= 0) {
    return 0;
  }

  for (Level &level : m_levels) {
    const qsizetype removeBuckets = drop / level.span;
    level.buckets.erase(level.buckets.begin(),
                        level.buckets.begin() + removeBuckets);
  }
  m_builtCount -= drop;
  return drop;
}

qsizetype LodPyramid::pickLevel(qsizetype spanNeeded) const {
  qsizetype k = 0;
  for (qsizetype i = 0; i < m_levels.size(); ++i) {
    if (m_levels[i].span <= spanNeeded) {
      k = i;
    } else {
      break;
    }
  }
  return k;
}

QVector<QPointF> LodPyramid::queryMinMax(qsizetype loIdx, qsizetype hiIdx,
                                         qsizetype maxBuckets) const {
  QVector<QPointF> out;
  if (m_levels.isEmpty() || hiIdx <= loIdx || maxBuckets <= 0) {
    return out;
  }

  const qsizetype visible = hiIdx - loIdx;
  const qsizetype spanNeeded = std::max<qsizetype>(1, visible / maxBuckets);
  const Level &level = m_levels[pickLevel(spanNeeded)];
  const qsizetype span = level.span;

  qsizetype bLo = loIdx / span;
  qsizetype bHi = (hiIdx + span - 1) / span;
  bHi = std::min(bHi, static_cast<qsizetype>(level.buckets.size()));
  bLo = std::min(bLo, bHi);

  out.reserve((bHi - bLo) * 2);
  for (qsizetype b = bLo; b < bHi; ++b) {
    const Bucket &bk = level.buckets[b];
    // Emit the two extremes in x-order so the envelope polyline never
    // backtracks.
    if (bk.lo.x() <= bk.hi.x()) {
      out.append(bk.lo);
      if (bk.hi != bk.lo) {
        out.append(bk.hi);
      }
    } else {
      out.append(bk.hi);
      out.append(bk.lo);
    }
  }

  return out;
}

QVector<QPointF> LodPyramid::queryLttb(qsizetype loIdx, qsizetype hiIdx,
                                       qsizetype targetPoints) const {
  QVector<QPointF> out;
  if (m_levels.isEmpty() || hiIdx <= loIdx || targetPoints <= 2) {
    return out;
  }

  // Oversample: read a finer level so LTTB has several candidate points per
  // output point to choose from (better shape than 1:1).
  constexpr qsizetype kOversample = 4;
  const qsizetype visible = hiIdx - loIdx;
  const qsizetype spanNeeded =
      std::max<qsizetype>(1, visible / (targetPoints * kOversample));
  const Level &level = m_levels[pickLevel(spanNeeded)];
  const qsizetype span = level.span;

  qsizetype bLo = loIdx / span;
  qsizetype bHi = (hiIdx + span - 1) / span;
  bHi = std::min(bHi, static_cast<qsizetype>(level.buckets.size()));
  bLo = std::min(bLo, bHi);

  // Flatten buckets to candidate points (both extremes, x-ordered). These are
  // globally ascending in x because buckets are index-ordered.
  QVector<QPointF> candidates;
  candidates.reserve((bHi - bLo) * 2);
  for (qsizetype b = bLo; b < bHi; ++b) {
    const Bucket &bk = level.buckets[b];
    if (bk.lo.x() <= bk.hi.x()) {
      candidates.append(bk.lo);
      if (bk.hi != bk.lo) {
        candidates.append(bk.hi);
      }
    } else {
      candidates.append(bk.hi);
      candidates.append(bk.lo);
    }
  }

  if (candidates.size() <= targetPoints) {
    return candidates;
  }

  // Reduce the candidates to the target with LTTB for the clean-line look.
  out.resize(targetPoints);
  LargestTriangleThreeBuckets lttb;
  lttb.downsample(candidates.constBegin(), candidates.size(), out.begin(),
                  targetPoints);
  return out;
}

} // namespace ChartPlotter

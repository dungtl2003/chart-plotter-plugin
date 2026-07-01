#include "ChartPlotter/utils/RenderMath.hpp"

namespace ChartPlotter {

float RenderMath::cross2D(const QVector2D &a, const QVector2D &b) {
  return a.x() * b.y() - a.y() * b.x();
}

QVector2D RenderMath::perpendicularLeft(const QVector2D &v) {
  return QVector2D(-v.y(), v.x());
}

bool RenderMath::lineIntersection(const QVector2D &p, const QVector2D &r,
                                  const QVector2D &q, const QVector2D &s,
                                  QVector2D &out) {
  // p + t*r = q + u*s

  const float rxs = cross2D(r, s);

  if (std::abs(rxs) < 0.000001f) {
    return false;
  }

  const QVector2D qmp = q - p;
  const float t = cross2D(qmp, s) / rxs;

  out = p + r * t;
  return true;
}

QVector2D RenderMath::lerpPoint(const QVector2D &a, const QVector2D &b,
                                float t) {
  return a + (b - a) * t;
}

bool RenderMath::nearlyEqual(const QVector2D &a, const QVector2D &b,
                             float eps) {
  return (a - b).lengthSquared() <= eps * eps;
}

double RenderMath::normalize(double value, double min, double max) {
  const double range = max - min;

  if (std::abs(range) < 1e-12) {
    return 0.5;
  }

  return (value - min) / range;
}

double RenderMath::niceNumber(double value, bool round) {
  double exponent = std::floor(std::log10(value));
  double fraction = value / std::pow(10.0, exponent);

  double niceFraction;

  if (round) {
    if (fraction < 1.5) {
      niceFraction = 1.0;
    } else if (fraction < 3.0) {
      niceFraction = 2.0;
    } else if (fraction < 7.0) {
      niceFraction = 5.0;
    } else {
      niceFraction = 10.0;
    }
  } else {
    if (fraction <= 1.0) {
      niceFraction = 1.0;
    } else if (fraction <= 2.0) {
      niceFraction = 2.0;
    } else if (fraction <= 5.0) {
      niceFraction = 5.0;
    } else {
      niceFraction = 10.0;
    }
  }

  return niceFraction * std::pow(10.0, exponent);
}

RenderMath::NiceBoundsResult
RenderMath::expandToNiceBounds(const DataRange &rawRange, int targetTickCount) {
  targetTickCount = std::max(targetTickCount, 2);

  double min = rawRange.min;
  double max = rawRange.max;

  if (!rawRange.valid || !std::isfinite(min) || !std::isfinite(max) ||
      min >= max) {
    min = -1.0;
    max = 1.0;
  }

  const double rawDiff = max - min;

  double niceRange = niceNumber(rawDiff, false);
  double step = niceNumber(niceRange / (targetTickCount - 1), true);

  // Guard a degenerate step: for an extremely small range, niceNumber can round
  // to zero (or a denormal), and dividing by it below yields inf/nan ticks and
  // an unbounded tick loop. Fall back to an even split of the (sanitized)
  // range.
  if (!std::isfinite(step) || step <= 0.0) {
    step = rawDiff / (targetTickCount - 1);
    if (!std::isfinite(step) || step <= 0.0) {
      step = 1.0;
    }
  }

  DataRange paddedRange;
  // Use the sanitized min/max (rawRange may have been invalid/non-finite
  // above).
  paddedRange.min = std::floor(min / step) * step;
  paddedRange.max = std::ceil(max / step) * step;
  paddedRange.valid = true;

  return NiceBoundsResult{.range = paddedRange, .step = step};
}

} // namespace ChartPlotter

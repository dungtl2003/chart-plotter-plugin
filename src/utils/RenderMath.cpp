#include "ChartPlotter/utils/RenderMath.hpp"

namespace ChartPlotter {

/**
 * This will tell you the direction of one relative to the other.
 *
 * Formular:
 * result = |a| * |b| * sin(θ) = a.x * b.y - a.y * b.x
 *
 * - sin(θ) < 0 -> θ > π -> b on the right of a
 * - sin(θ) > 0 -> θ < π -> b on the left of a
 * - sin(θ) = 0 -> θ = π or θ = 0 -> b and a have the same/oposite direction
 *   (maybe they are on the same line or parallel)
 */
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

} // namespace ChartPlotter

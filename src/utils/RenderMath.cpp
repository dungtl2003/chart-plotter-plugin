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

} // namespace ChartPlotter

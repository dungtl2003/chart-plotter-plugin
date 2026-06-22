#pragma once

#include "ChartPlotter/types/DataRange.hpp"

#include <QVector2D>

namespace ChartPlotter {

namespace RenderMath {

constexpr float Epsilon = 0.0001f;

struct NiceBoundsResult {
  DataRange range;
  double step = 0.0;
};

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
float cross2D(const QVector2D &a, const QVector2D &b);
/**
 * Find the intersection point between 2 lines.
 *
 * Formular: p + t*r = q + u*s
 */
QVector2D perpendicularLeft(const QVector2D &v);
bool lineIntersection(const QVector2D &p, const QVector2D &r,
                      const QVector2D &q, const QVector2D &s, QVector2D &out);
/**
 * Linear Interpolation - Phép nội suy tuyến tính
 *
 * Math function to find point between 2 given points (or values).
 *
 * Formular: V = A + (B - A) * t
 * Args:
 * - A: start point
 * - B: end point
 * - t: weight, 0 <= t <= 1
 *
 * Let's say t = 0.5, that means V is in the middle of A and B.
 */
QVector2D lerpPoint(const QVector2D &a, const QVector2D &b, float t);
// Check to see if 2 points are in the same position or VERY CLOSE to each
// other.
bool nearlyEqual(const QVector2D &a, const QVector2D &b, float eps = Epsilon);
/**
 * Scale value into 0 - 1 range.
 *
 * Formular:
 * x_normalize = (x - x_min) / (x_max - x_min)
 */
double normalize(double value, double min, double max);
template <typename T>
void appendQuad(QVector<T> &out, const T &v0, const T &v1, const T &v2,
                const T &v3);
template <typename T>
void appendTriangle(QVector<T> &out, const T &v0, const T &v1, const T &v2);

/**
 * It will try to round the value to one of these value:
 * 1 * 10^n
 * 2 * 10^n
 * 5 * 10^n
 * 10 * 10^n
 *
 * First, calculate exponent: 10^n = value -> n = log10(value)
 * Then calculate the fraction: fraction = value / 10^n
 * fraction can only in range (0, 10), so we can round (cell) fraction on that
 * range to find suitable nice number.
 *
 * nice number = nice fraction * 10^n
 */
double niceNumber(double value, bool round);

NiceBoundsResult expandToNiceBounds(const DataRange &rawRange,
                                    int targetTickCount);

} // namespace RenderMath

} // namespace ChartPlotter

#include "ChartPlotter/utils/RenderMath.tpp"

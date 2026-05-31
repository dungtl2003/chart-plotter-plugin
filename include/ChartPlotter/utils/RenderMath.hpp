#pragma once

#include <QVector2D>

namespace ChartPlotter {

namespace RenderMath {

float cross2D(const QVector2D &a, const QVector2D &b);
QVector2D perpendicularLeft(const QVector2D &v);
bool lineIntersection(const QVector2D &p, const QVector2D &r,
                      const QVector2D &q, const QVector2D &s, QVector2D &out);

} // namespace RenderMath

} // namespace ChartPlotter

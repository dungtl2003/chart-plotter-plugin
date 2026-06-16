#pragma once

#include <QVector>

namespace ChartPlotter {

namespace RenderMath {

template <typename T>
void appendQuad(QVector<T> &out, const T &v0, const T &v1, const T &v2,
                const T &v3) {
  out.push_back(v0);
  out.push_back(v1);
  out.push_back(v2);

  out.push_back(v2);
  out.push_back(v1);
  out.push_back(v3);
}

template <typename T>
void appendTriangle(QVector<T> &out, const T &v0, const T &v1, const T &v2) {
  out.push_back(v0);
  out.push_back(v1);
  out.push_back(v2);
}

} // namespace RenderMath

} // namespace ChartPlotter

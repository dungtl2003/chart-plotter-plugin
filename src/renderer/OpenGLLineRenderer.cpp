#include "ChartPlotter/renderer/OpenGLLineRenderer.hpp"
#include "ChartPlotter/utils/Gl.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "ChartPlotter/utils/RenderMath.hpp"

namespace ChartPlotter {

namespace {

constexpr int MarkerSegments = 32;

// Builds an oriented quad that fully covers the capsule (segment + radius),
// extended along the segment by coverRadius at both ends for round caps.
// Each corner carries the true (p0, p1) so the fragment shader can measure
// distance to the centerline. p0 == p1 yields a square -> the SDF becomes a
// disc.
void appendCapsule(QVector<LineStrokeVertex> &out, const QVector2D &p0,
                   const QVector2D &p1, float coverRadius) {
  QVector2D delta = p1 - p0;

  QVector2D dir;
  if (delta.lengthSquared() <= RenderMath::Epsilon * RenderMath::Epsilon) {
    dir =
        QVector2D(1.0f, 0.0f); // degenerate (dot): any orthonormal basis works
  } else {
    dir = delta.normalized();
  }

  const QVector2D n(-dir.y(), dir.x());
  const QVector2D ext = dir * coverRadius; // along-axis extension (caps)
  const QVector2D off = n * coverRadius;   // perpendicular extension

  const QVector2D a = p0 - ext;
  const QVector2D b = p1 + ext;

  // Same corner roles as the old appendStrokeQuad, so winding is unchanged.
  const LineStrokeVertex c0{a - off, p0, p1};
  const LineStrokeVertex c1{a + off, p0, p1};
  const LineStrokeVertex c2{b - off, p0, p1};
  const LineStrokeVertex c3{b + off, p0, p1};

  RenderMath::appendQuad(out, c0, c1, c2, c3);
}

void appendUniquePoint(QVector<QVector2D> &pts, const QVector2D &p) {
  if (pts.isEmpty() || !RenderMath::nearlyEqual(pts.back(), p)) {
    pts.push_back(p);
  }
}

QPointF mapDataToItem(const QPointF &point, const AxisRange &xRange,
                      const AxisRange &yRange, const QRectF &plotArea) {
  const double nx = RenderMath::normalize(point.x(), xRange.min, xRange.max);
  const double ny = RenderMath::normalize(point.y(), yRange.min, yRange.max);

  const double x = plotArea.left() + nx * plotArea.width();

  // invert y axis
  const double y = plotArea.bottom() - ny * plotArea.height();

  return QPointF(x, y);
}

void appendStrokeQuad(QVector<LineStrokeVertex> &outVertices,
                      const QVector2D &p0, const QVector2D &p1,
                      float halfWidth) {
  QVector2D dir = p1 - p0;

  if (dir.lengthSquared() <= 0.0001f) {
    return;
  }

  dir.normalize();

  const QVector2D n(-dir.y(), dir.x());

  const LineStrokeVertex v0{p0 - n * halfWidth};
  const LineStrokeVertex v1{p0 + n * halfWidth};
  const LineStrokeVertex v2{p1 - n * halfWidth};
  const LineStrokeVertex v3{p1 + n * halfWidth};

  RenderMath::appendQuad(outVertices, v0, v1, v2, v3);
}

QVector<DashRun> buildDashRuns(const QVector<QVector2D> &points,
                               float dashLength, float gapLength) {
  QVector<DashRun> runs;
  // CP_DEBUG("gapLength = {}", gapLength);

  if (points.size() < 2) {
    return runs;
  }

  const float patternLength = dashLength + gapLength;
  if (dashLength <= 0.0f || gapLength < 0.0f || patternLength <= 0.0f) {
    return runs;
  }

  float distanceAlongPolyline = 0.0f;

  DashRun currentRun;
  bool runOpen = false;

  for (int i = 0; i + 1 < points.size(); ++i) {
    const QVector2D a = points[i];
    const QVector2D b = points[i + 1];

    QVector2D seg = b - a;
    const float segLen = seg.length();

    if (segLen <= RenderMath::Epsilon) {
      continue;
    }

    float distanceOnSegment = 0.0f;

    while (distanceOnSegment < segLen) {
      const float globalDistance = distanceAlongPolyline + distanceOnSegment;
      const float patternPos = std::fmod(globalDistance, patternLength);
      const bool insideDash = patternPos < dashLength;

      const float remainingInPattern =
          insideDash ? (dashLength - patternPos) : (patternLength - patternPos);

      const float step =
          std::min(remainingInPattern, segLen - distanceOnSegment);

      const float t0 = distanceOnSegment / segLen;
      const float t1 = (distanceOnSegment + step) / segLen;

      const QVector2D p0 = RenderMath::lerpPoint(a, b, t0);
      const QVector2D p1 = RenderMath::lerpPoint(a, b, t1);

      if (insideDash && step > RenderMath::Epsilon) {
        if (!runOpen) {
          currentRun.points.clear();
          appendUniquePoint(currentRun.points, p0);
          runOpen = true;
        }

        appendUniquePoint(currentRun.points, p1);
      } else {
        if (runOpen && currentRun.points.size() >= 2) {
          runs.push_back(currentRun);
        }

        currentRun.points.clear();
        runOpen = false;
      }

      distanceOnSegment += step;
    }

    distanceAlongPolyline += segLen;
  }

  if (runOpen && currentRun.points.size() >= 2) {
    runs.push_back(currentRun);
  }

  return runs;
}

} // namespace

void OpenGLLineRenderer::initialize(QOpenGLExtraFunctions *f) {
  CP_DEBUG("OpenGLLineRenderer::initialize: Initing OpenGL resources...");

  initializePrograms();
  initializeStrokeGeometry(f);
  initializeMarkerGeometry(f);
}

void OpenGLLineRenderer::release(QOpenGLExtraFunctions *f) {
  CP_DEBUG("OpenGLLineRenderer::release: Releasing OpenGL resources...");

  if (m_strokeVbo) {
    f->glDeleteBuffers(1, &m_strokeVbo);
    m_strokeVbo = 0;
  }
  if (m_strokeVao) {
    f->glDeleteVertexArrays(1, &m_strokeVao);
    m_strokeVao = 0;
  }

  if (m_markerVbo) {
    f->glDeleteBuffers(1, &m_markerVbo);
    m_markerVbo = 0;
  }

  if (m_markerVao) {
    f->glDeleteVertexArrays(1, &m_markerVao);
    m_markerVao = 0;
  }

  m_strokeProgram.reset();
  m_markerProgram.reset();
}

void OpenGLLineRenderer::render(const ChartRenderContext &context) {
  if (!m_strokeProgram || !m_data) {
    return;
  }

  LineRenderData *lineData = m_data.get();
  QOpenGLExtraFunctions *f = context.f;
  const QMatrix4x4 &mvp = context.mvp;

  const double halfWidth = lineData->stroke.width * 0.5f;
  const AxisRange xRange = context.xAxisRange;
  const AxisRange yRange = context.yAxisRange;

  QVector<QVector2D> points;
  points.reserve(lineData->points.size());

  for (const auto &p : lineData->points) {
    points.push_back(
        QVector2D(mapDataToItem(p, xRange, yRange, context.plotArea)));
  }

  buildStrokeVertices(points);
  buildMarkerVertices(points);

  drawStrokes(f, mvp);

  if (lineData->marker.visible) {
    drawMarkers(f, mvp);
  }
}

void OpenGLLineRenderer::setData(std::unique_ptr<RenderData> data) {
  if (LineRenderData *lineData = dynamic_cast<LineRenderData *>(data.get())) {
    auto _ =
        data.release(); // so it does not track and auto remove the data inside
    m_data = std::unique_ptr<LineRenderData>(lineData);
  }
};

void OpenGLLineRenderer::buildStrokeVertices(const QVector<QVector2D> &points) {
  assert(m_data != nullptr);
  m_strokeVertices.clear();

  const auto *lineData = m_data.get();
  switch (lineData->stroke.pattern) {
  case ChartEnums::StrokePattern::Solid:
    buildSolidStrokeVertices(points, m_strokeVertices);
    break;
  case ChartEnums::StrokePattern::Dash:
    buildDashedStrokeVertices(points, m_strokeVertices);
    break;
  case ChartEnums::StrokePattern::Dot:
    buildDottedStrokeVertices(points, m_strokeVertices);
    break;
  }
}

void OpenGLLineRenderer::buildSolidStrokeVertices(
    const QVector<QVector2D> &points, QVector<LineStrokeVertex> &outVertices) {
  assert(m_data != nullptr);
  if (points.size() < 2) {
    return;
  }

  const auto *lineData = m_data.get();
  const float halfWidth = lineData->stroke.width * 0.5f;
  const float coverRadius = halfWidth + lineData->antialias + 1.0f;

  for (int i = 0; i + 1 < points.size(); ++i) {
    appendCapsule(outVertices, points[i], points[i + 1], coverRadius);
  }
}

void OpenGLLineRenderer::buildDashedStrokeVertices(
    const QVector<QVector2D> &points, QVector<LineStrokeVertex> &outVertices) {
  assert(m_data != nullptr);
  const auto *lineData = m_data.get();

  const float dashLength =
      lineData->stroke.dashStyle.length - lineData->stroke.width;
  // shader will add more length than normal (2 * halfWidth)
  const float gapLength =
      lineData->stroke.dashStyle.gap + lineData->stroke.width;
  const float halfWidth = lineData->stroke.width * 0.5f;
  const float coverRadius = halfWidth + lineData->antialias + 1.0f;

  const QVector<DashRun> runs = buildDashRuns(points, dashLength, gapLength);
  for (const DashRun &run : runs) {
    for (int i = 0; i + 1 < run.points.size(); ++i) {
      appendCapsule(outVertices, run.points[i], run.points[i + 1], coverRadius);
    }
  }
}

void OpenGLLineRenderer::buildDottedStrokeVertices(
    const QVector<QVector2D> &points, QVector<LineStrokeVertex> &outVertices) {
  assert(m_data != nullptr);
  const auto *lineData = m_data.get();

  if (points.size() < 2) {
    return;
  }

  const float radius = lineData->stroke.width * 0.5f;
  const float gap = lineData->stroke.dotStyle.gap;
  const float stepLength = radius * 2.0f + gap;
  const float coverRadius = radius + lineData->antialias + 1.0f;

  if (radius <= RenderMath::Epsilon || stepLength <= RenderMath::Epsilon) {
    return;
  }

  float nextDotDistance = 0.0f;
  float distanceAlongPolyline = 0.0f;

  for (int i = 0; i + 1 < points.size(); ++i) {
    const QVector2D segmentStart = points[i];
    const QVector2D segmentEnd = points[i + 1];

    const QVector2D segment = segmentEnd - segmentStart;
    const float segmentLength = segment.length();

    if (segmentLength <= RenderMath::Epsilon) {
      continue;
    }

    while (nextDotDistance <= distanceAlongPolyline + segmentLength) {
      const float distanceOnSegment = nextDotDistance - distanceAlongPolyline;

      if (distanceOnSegment >= -RenderMath::Epsilon &&
          distanceOnSegment <= segmentLength + RenderMath::Epsilon) {
        const float t = distanceOnSegment / segmentLength;
        const QVector2D center =
            RenderMath::lerpPoint(segmentStart, segmentEnd, t);

        appendCapsule(outVertices, center, center, coverRadius); // -> disc
      }

      nextDotDistance += stepLength;
    }

    distanceAlongPolyline += segmentLength;
  }
}

void OpenGLLineRenderer::buildMarkerVertices(const QVector<QVector2D> &points) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  m_markerVertices.clear();

  const float radius = lineData->stroke.width;
  const float outerRadius = radius + lineData->antialias;

  for (const QVector2D &center : points) {
    const QVector2D p0 = center + QVector2D(-outerRadius, -outerRadius);
    const QVector2D p1 = center + QVector2D(+outerRadius, -outerRadius);
    const QVector2D p2 = center + QVector2D(-outerRadius, +outerRadius);
    const QVector2D p3 = center + QVector2D(+outerRadius, +outerRadius);

    RenderMath::appendQuad(m_markerVertices,
                           MarkerVertex{.position = p0, .center = center},
                           MarkerVertex{.position = p1, .center = center},
                           MarkerVertex{.position = p2, .center = center},
                           MarkerVertex{.position = p3, .center = center});
  }
}

void OpenGLLineRenderer::uploadStrokeVertices(QOpenGLExtraFunctions *f) {
  f->glBindBuffer(GL_ARRAY_BUFFER, m_strokeVbo);

  f->glBufferData(GL_ARRAY_BUFFER,
                  static_cast<GLsizeiptr>(m_strokeVertices.size() *
                                          sizeof(LineStrokeVertex)),
                  m_strokeVertices.data(), GL_DYNAMIC_DRAW);

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLLineRenderer::uploadMarkerVertices(QOpenGLExtraFunctions *f) {
  f->glBindBuffer(GL_ARRAY_BUFFER, m_markerVbo);

  f->glBufferData(
      GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(m_markerVertices.size() * sizeof(MarkerVertex)),
      m_markerVertices.data(), GL_DYNAMIC_DRAW);

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLLineRenderer::drawStrokesAsTriangles(QOpenGLExtraFunctions *f) {
  if (m_strokeVertices.empty()) {
    return;
  }

  f->glBindVertexArray(m_strokeVao);
  f->glDrawArrays(GL_TRIANGLES, 0,
                  static_cast<GLsizei>(m_strokeVertices.size()));
  f->glBindVertexArray(0);
}

void OpenGLLineRenderer::drawStrokes(QOpenGLExtraFunctions *f,
                                     const QMatrix4x4 &mvp) {
  if (!m_strokeProgram || m_strokeVertices.empty() || !m_data) {
    return;
  }

  f->glBindBuffer(GL_ARRAY_BUFFER, m_strokeVbo);
  f->glBufferData(GL_ARRAY_BUFFER,
                  static_cast<GLsizeiptr>(m_strokeVertices.size() *
                                          sizeof(LineStrokeVertex)),
                  m_strokeVertices.data(), GL_DYNAMIC_DRAW);
  f->glBindBuffer(GL_ARRAY_BUFFER, 0);

  const auto &lineData = m_data.get();
  m_strokeProgram->bind();
  m_strokeProgram->setUniformValue("u_mvp", mvp);
  m_strokeProgram->setUniformValue("u_color", lineData->stroke.color);
  m_strokeProgram->setUniformValue("u_antialias", lineData->antialias);
  m_strokeProgram->setUniformValue("u_halfWidth",
                                   lineData->stroke.width * 0.5f);

  f->glBindVertexArray(m_strokeVao);
  f->glDrawArrays(GL_TRIANGLES, 0,
                  static_cast<GLsizei>(m_strokeVertices.size()));
  f->glBindVertexArray(0);

  m_strokeProgram->release();
}

void OpenGLLineRenderer::drawMarkers(QOpenGLExtraFunctions *f,
                                     const QMatrix4x4 &mvp) {
  if (!m_markerProgram || m_markerVertices.empty() || !m_data) {
    return;
  }

  f->glBindBuffer(GL_ARRAY_BUFFER, m_markerVbo);
  f->glBufferData(
      GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(m_markerVertices.size() * sizeof(MarkerVertex)),
      m_markerVertices.data(), GL_DYNAMIC_DRAW);
  f->glBindBuffer(GL_ARRAY_BUFFER, 0);

  const auto &lineData = m_data.get();
  m_markerProgram->bind();
  m_markerProgram->setUniformValue("u_mvp", mvp);
  m_markerProgram->setUniformValue("u_color", lineData->marker.color);
  m_markerProgram->setUniformValue("u_radius", lineData->stroke.width);
  m_markerProgram->setUniformValue("u_antialias", lineData->antialias);

  f->glBindVertexArray(m_markerVao);
  f->glDrawArrays(GL_TRIANGLES, 0,
                  static_cast<GLsizei>(m_markerVertices.size()));
  f->glBindVertexArray(0);

  m_markerProgram->release();
}

void OpenGLLineRenderer::drawMarkersAsTriangles(QOpenGLExtraFunctions *f) {
  if (m_markerVertices.empty()) {
    return;
  }

  f->glBindVertexArray(m_markerVao);
  f->glDrawArrays(GL_TRIANGLES, 0,
                  static_cast<GLsizei>(m_markerVertices.size()));
  f->glBindVertexArray(0);
}

void OpenGLLineRenderer::bindStrokeProgram(const QMatrix4x4 &mvp) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  m_strokeProgram->bind();
  m_strokeProgram->setUniformValue("u_mvp", mvp);
  m_strokeProgram->setUniformValue("u_color", lineData->stroke.color);
  m_strokeProgram->setUniformValue("u_antialias", lineData->antialias);
  m_strokeProgram->setUniformValue("u_halfWidth",
                                   lineData->stroke.width * 0.5f);
}

void OpenGLLineRenderer::bindMarkerProgram(const QMatrix4x4 &mvp) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  m_markerProgram->bind();
  m_markerProgram->setUniformValue("u_mvp", mvp);
  m_markerProgram->setUniformValue("u_color", lineData->marker.color);
  m_markerProgram->setUniformValue("u_radius", lineData->stroke.width);
  m_markerProgram->setUniformValue("u_antialias", lineData->antialias);
}

void OpenGLLineRenderer::initializeStrokeGeometry(QOpenGLExtraFunctions *f) {
  f->glGenVertexArrays(1, &m_strokeVao);
  f->glGenBuffers(1, &m_strokeVbo);

  f->glBindVertexArray(m_strokeVao);
  f->glBindBuffer(GL_ARRAY_BUFFER, m_strokeVbo);

  f->glEnableVertexAttribArray(0);
  f->glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, sizeof(LineStrokeVertex),
      reinterpret_cast<void *>(offsetof(LineStrokeVertex, position)));

  f->glEnableVertexAttribArray(1);
  f->glVertexAttribPointer(
      1, 2, GL_FLOAT, GL_FALSE, sizeof(LineStrokeVertex),
      reinterpret_cast<void *>(offsetof(LineStrokeVertex, p0)));

  f->glEnableVertexAttribArray(2);
  f->glVertexAttribPointer(
      2, 2, GL_FLOAT, GL_FALSE, sizeof(LineStrokeVertex),
      reinterpret_cast<void *>(offsetof(LineStrokeVertex, p1)));

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
  f->glBindVertexArray(0);
}

void OpenGLLineRenderer::initializeMarkerGeometry(QOpenGLExtraFunctions *f) {
  f->glGenVertexArrays(1, &m_markerVao);
  f->glGenBuffers(1, &m_markerVbo);

  f->glBindVertexArray(m_markerVao);
  f->glBindBuffer(GL_ARRAY_BUFFER, m_markerVbo);

  f->glEnableVertexAttribArray(0);
  f->glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, sizeof(MarkerVertex),
      reinterpret_cast<void *>(offsetof(MarkerVertex, position)));

  f->glEnableVertexAttribArray(1);
  f->glVertexAttribPointer(
      1, 2, GL_FLOAT, GL_FALSE, sizeof(MarkerVertex),
      reinterpret_cast<void *>(offsetof(MarkerVertex, center)));

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
  f->glBindVertexArray(0);
}

void OpenGLLineRenderer::initializePrograms() {
  m_strokeProgram = std::make_unique<QOpenGLShaderProgram>();
  m_markerProgram = std::make_unique<QOpenGLShaderProgram>();

  const QString strokeVertexShader =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/line/stroke.vert");
  const QString strokeFragmentShader =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/line/stroke.frag");
  const QString markerVertexShader =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/line/marker.vert");
  const QString markerFragmentShader =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/line/marker.frag");

  m_strokeProgram = Gl::createProgram(strokeVertexShader, strokeFragmentShader,
                                      "StrokeProgram");
  m_markerProgram = Gl::createProgram(markerVertexShader, markerFragmentShader,
                                      "MarkerProgram");
  if (!m_strokeProgram || !m_markerProgram) {
    CP_WARN("OpenGLLineRenderer::initialize: failed to create shader programs");
    return;
  }
}

} // namespace ChartPlotter

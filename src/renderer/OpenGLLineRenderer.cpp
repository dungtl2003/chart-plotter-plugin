#include "ChartPlotter/renderer/OpenGLLineRenderer.hpp"
#include "ChartPlotter/utils/Gl.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "ChartPlotter/utils/RenderMath.hpp"

namespace ChartPlotter {

namespace {

constexpr int MarkerSegments = 32;

void appendUniquePoint(QVector<QVector2D> &pts, const QVector2D &p) {
  if (pts.isEmpty() || !RenderMath::nearlyEqual(pts.back(), p)) {
    pts.push_back(p);
  }
}

QPointF mapDataToItem(const QPointF &point, const ValueAxisRange &xRange,
                      const ValueAxisRange &yRange, const QRectF &plotArea) {
  const double nx = RenderMath::normalize(point.x(), xRange.min, xRange.max);
  const double ny = RenderMath::normalize(point.y(), yRange.min, yRange.max);

  const double x = plotArea.left() + nx * plotArea.width();

  // invert y axis
  const double y = plotArea.bottom() - ny * plotArea.height();

  return QPointF(x, y);
}

void appendStrokeQuad(QVector<StrokeVertex> &outVertices, const QVector2D &p0,
                      const QVector2D &p1, float halfWidth) {
  QVector2D dir = p1 - p0;

  if (dir.lengthSquared() <= 0.0001f) {
    return;
  }

  dir.normalize();

  const QVector2D n(-dir.y(), dir.x());

  const StrokeVertex v0{p0 - n * halfWidth};
  const StrokeVertex v1{p0 + n * halfWidth};
  const StrokeVertex v2{p1 - n * halfWidth};
  const StrokeVertex v3{p1 + n * halfWidth};

  RenderMath::appendQuad(outVertices, v0, v1, v2, v3);
}

void appendStrokeCircle(QVector<StrokeVertex> &outVertices,
                        const QVector2D &center, float radius) {
  if (radius <= RenderMath::Epsilon) {
    return;
  }

  for (int i = 0; i < MarkerSegments; ++i) {
    const float a0 = static_cast<float>(i) /
                     static_cast<float>(MarkerSegments) * 2.0f * M_PI;

    const float a1 = static_cast<float>(i + 1) /
                     static_cast<float>(MarkerSegments) * 2.0f * M_PI;

    const QVector2D p0 = center;

    const QVector2D p1 =
        center + QVector2D(std::cos(a0) * radius, std::sin(a0) * radius);

    const QVector2D p2 =
        center + QVector2D(std::cos(a1) * radius, std::sin(a1) * radius);

    RenderMath::appendTriangle(outVertices, StrokeVertex{p0}, StrokeVertex{p1},
                               StrokeVertex{p2});
  }
}

QVector<DashRun> buildDashRuns(const QVector<QVector2D> &points,
                               float dashLength, float gapLength) {
  QVector<DashRun> runs;

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
  const ValueAxisRange xRange = context.xAxisRange;
  const ValueAxisRange yRange = context.yAxisRange;

  QVector<QVector2D> points;
  points.reserve(lineData->points.size());

  for (const auto &p : lineData->points) {
    points.push_back(
        QVector2D(mapDataToItem(p, xRange, yRange, context.plotArea)));
  }

  buildStrokeVertices(points);
  buildMarkerVertices(points);

  uploadStrokeVertices(f);
  uploadMarkerVertices(f);

  f->glEnable(GL_BLEND);
  f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  bindStrokeProgram(mvp);
  drawStrokesAsTriangles(f);
  m_strokeProgram->release();

  if (lineData->marker.visible) {
    bindMarkerProgram(mvp);
    drawMarkersAsTriangles(f);
    m_markerProgram->release();
  }

  f->glDisable(GL_BLEND);
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
  const auto &lineData = m_data.get();

  m_strokeVertices.clear();

  switch (lineData->stroke.pattern) {
  case ChartEnums::StrokePattern::Solid:
    buildSegmentVertices(points, m_strokeVertices);
    buildMiterJoinVertices(points, m_strokeVertices);
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
    const QVector<QVector2D> &points) {
  m_strokeVertices.clear();

  buildSegmentVertices(points, m_strokeVertices);
  buildMiterJoinVertices(points, m_strokeVertices);
}

void OpenGLLineRenderer::buildDashedStrokeVertices(
    const QVector<QVector2D> &points, QVector<StrokeVertex> &outVertices) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  const float dashLength = lineData->stroke.dashStyle.length;
  const float gapLength = lineData->stroke.dashStyle.gap;

  QVector<DashRun> runs = buildDashRuns(points, dashLength, gapLength);

  for (const DashRun &run : runs) {
    if (run.points.size() < 2) {
      continue;
    }

    buildSegmentVertices(run.points, outVertices);
    buildMiterJoinVertices(run.points, outVertices);
  }
}

void OpenGLLineRenderer::buildDottedStrokeVertices(
    const QVector<QVector2D> &points, QVector<StrokeVertex> &outVertices) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  if (points.size() < 2) {
    return;
  }

  const float radius = lineData->stroke.width * 0.5f;
  const float gap = lineData->stroke.dotStyle.gap;
  const float stepLength = radius * 2.0f + gap;

  if (radius <= RenderMath::Epsilon || stepLength <= RenderMath::Epsilon) {
    return;
  }

  float nextDotDistance = 0.0f;
  float distanceAlongPolyline = 0.0f;

  for (int i = 0; i + 1 < points.size(); ++i) {
    const QVector2D segmentStart = points[i];
    const QVector2D segmentEnd = points[i + 1];

    QVector2D segment = segmentEnd - segmentStart;
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

        appendStrokeCircle(outVertices, center, radius);
      }

      nextDotDistance += stepLength;
    }

    distanceAlongPolyline += segmentLength;
  }
}

// Quad Extend
void OpenGLLineRenderer::buildSegmentVertices(
    const QVector<QVector2D> &points, QVector<StrokeVertex> &outVertices) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  if (points.size() < 2) {
    return;
  }

  const float halfWidth = lineData->stroke.width * 0.5f;

  for (std::size_t i = 0; i + 1 < points.size(); ++i) {
    appendStrokeQuad(outVertices, points[i], points[i + 1], halfWidth);
  }
}

void OpenGLLineRenderer::buildMiterJoinVertices(
    const QVector<QVector2D> &points, QVector<StrokeVertex> &outVertices) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  if (points.size() < 3) {
    return;
  }

  const float halfWidth = lineData->stroke.width * 0.5f;

  for (std::size_t i = 1; i + 1 < points.size(); ++i) {
    QVector2D p0 = points[i - 1];
    QVector2D p1 = points[i];
    QVector2D p2 = points[i + 1];

    QVector2D dirA = p1 - p0;
    QVector2D dirB = p2 - p1;

    if (dirA.lengthSquared() <= 0.0001f || dirB.lengthSquared() <= 0.0001f) {
      continue;
    }

    dirA.normalize();
    dirB.normalize();

    float turn = RenderMath::cross2D(dirA, dirB);

    if (std::abs(turn) <= 0.0001f) {
      continue;
    }

    QVector2D normalA = RenderMath::perpendicularLeft(dirA);
    QVector2D normalB = RenderMath::perpendicularLeft(dirB);

    QVector2D sideA;
    QVector2D sideB;

    // For Qt y-down setup.
    if (turn > 0.0f) {
      sideA = -normalA;
      sideB = -normalB;
    } else {
      sideA = normalA;
      sideB = normalB;
    }

    QVector2D edgeA = p1 + sideA * halfWidth;
    QVector2D edgeB = p1 + sideB * halfWidth;

    QVector2D miterPoint;

    bool ok =
        RenderMath::lineIntersection(edgeA, dirA, edgeB, dirB, miterPoint);

    if (!ok) {
      continue;
    }

    float miterLength = (miterPoint - p1).length();
    bool useMiter = (miterLength / halfWidth) <= lineData->stroke.miterLimit &&
                    !lineData->marker.visible;

    if (useMiter) {
      RenderMath::appendQuad(outVertices, StrokeVertex{edgeA}, StrokeVertex{p1},
                             StrokeVertex{miterPoint}, StrokeVertex{edgeB});
    } else {
      RenderMath::appendTriangle(outVertices, {p1}, {edgeA}, {edgeB});
    }
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
                           MarkerSdfVertex{.position = p0, .center = center},
                           MarkerSdfVertex{.position = p1, .center = center},
                           MarkerSdfVertex{.position = p2, .center = center},
                           MarkerSdfVertex{.position = p3, .center = center});
  }
}

void OpenGLLineRenderer::uploadStrokeVertices(QOpenGLExtraFunctions *f) {
  f->glBindBuffer(GL_ARRAY_BUFFER, m_strokeVbo);

  f->glBufferData(
      GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(m_strokeVertices.size() * sizeof(StrokeVertex)),
      m_strokeVertices.data(), GL_DYNAMIC_DRAW);

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLLineRenderer::uploadMarkerVertices(QOpenGLExtraFunctions *f) {
  f->glBindBuffer(GL_ARRAY_BUFFER, m_markerVbo);

  f->glBufferData(GL_ARRAY_BUFFER,
                  static_cast<GLsizeiptr>(m_markerVertices.size() *
                                          sizeof(MarkerSdfVertex)),
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
      0, 2, GL_FLOAT, GL_FALSE, sizeof(StrokeVertex),
      reinterpret_cast<void *>(offsetof(StrokeVertex, position)));

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
      0, 2, GL_FLOAT, GL_FALSE, sizeof(MarkerSdfVertex),
      reinterpret_cast<void *>(offsetof(MarkerSdfVertex, position)));

  f->glEnableVertexAttribArray(1);
  f->glVertexAttribPointer(
      1, 2, GL_FLOAT, GL_FALSE, sizeof(MarkerSdfVertex),
      reinterpret_cast<void *>(offsetof(MarkerSdfVertex, center)));

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
  const QString markerVertexShader = Gl::readShaderSource(
      ":/qt/qml/ChartPlotter/shaders/line/marker-sdf-2.vert");
  const QString markerFragmentShader = Gl::readShaderSource(
      ":/qt/qml/ChartPlotter/shaders/line/marker-sdf-2.frag");

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

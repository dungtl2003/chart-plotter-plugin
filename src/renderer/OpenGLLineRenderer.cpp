#include "ChartPlotter/renderer/OpenGLLineRenderer.hpp"
#include "ChartPlotter/utils/Gl.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "ChartPlotter/utils/RenderMath.hpp"

namespace ChartPlotter {

namespace {

constexpr int MarkerSegments = 32;

double normalize(double value, double min, double max) {
  const double range = max - min;

  if (std::abs(range) < 1e-12) {
    return 0.5;
  }

  return (value - min) / range;
}

QPointF mapDataToItem(const QPointF &point, const ValueAxisRange &xRange,
                      const ValueAxisRange &yRange, const QRectF &plotArea) {
  const double nx = normalize(point.x(), xRange.min, xRange.max);
  const double ny = normalize(point.y(), yRange.min, yRange.max);

  const double x = plotArea.left() + nx * plotArea.width();

  // invert y axis
  const double y = plotArea.bottom() - ny * plotArea.height();

  return QPointF(x, y);
}

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

  /**
   *
   *                                                    half_width
   *       0                             half_width    +antialias
   *       ├─────────────────────────────────┼──────────────┼─────────────────
   *              fully visible                has opacity    fully invisible
   */
  const double halfWidth = m_data.get()->stroke.width * 0.5f;
  const ValueAxisRange xRange = context.xAxisRange;
  const ValueAxisRange yRange = context.yAxisRange;

  QVector<QVector2D> points;
  points.reserve(m_data.get()->points.size());

  for (const auto &p : m_data.get()->points) {
    points.push_back(
        QVector2D(mapDataToItem(p, xRange, yRange, context.plotArea)));
  }

  buildStrokeVertices(points);
  buildMarkerVertices(points);

  uploadStrokeVertices(f);
  uploadMarkerVertices(f);

  // f->glEnable(GL_BLEND);
  // f->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
  //                        GL_ONE_MINUS_SRC_ALPHA);

  bindStrokeProgram(mvp);
  drawStrokesAsTriangles(f);
  m_strokeProgram->release();

  if (lineData->marker.visible) {
    bindMarkerProgram(mvp);
    drawMarkersAsTriangles(f);
    m_markerProgram->release();
  }

  // f->glDisable(GL_BLEND);
}

void OpenGLLineRenderer::setData(std::unique_ptr<RenderData> data) {
  if (LineRenderData *lineData = dynamic_cast<LineRenderData *>(data.get())) {
    auto _ =
        data.release(); // so it does not track and auto remove the data inside
    m_data = std::unique_ptr<LineRenderData>(lineData);
  }
};

void OpenGLLineRenderer::buildStrokeVertices(const QVector<QVector2D> &points) {
  m_strokeVertices.clear();

  buildSegmentVertices(points, m_strokeVertices);
  buildMiterJoinVertices(points, m_strokeVertices);
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
    QVector2D p0 = points[i];
    QVector2D p1 = points[i + 1];

    QVector2D dir = p1 - p0;

    if (dir.lengthSquared() <= 0.0001f) {
      continue;
    }

    dir.normalize();

    QVector2D n(-dir.y(), dir.x());

    StrokeVertex v0 = StrokeVertex{p0 - n * halfWidth};
    StrokeVertex v1 = StrokeVertex{p0 + n * halfWidth};
    StrokeVertex v2 = StrokeVertex{p1 - n * halfWidth};
    StrokeVertex v3 = StrokeVertex{p1 + n * halfWidth};

    appendQuad(outVertices, v0, v1, v2, v3);
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
    bool useMiter =
        (miterLength / halfWidth) <= 4.0f && !lineData->marker.visible;

    if (useMiter) {
      appendQuad(outVertices, StrokeVertex{edgeA}, StrokeVertex{p1},
                 StrokeVertex{miterPoint}, StrokeVertex{edgeB});
    } else {
      appendTriangle(outVertices, {p1}, {edgeA}, {edgeB});
    }
  }
}

void OpenGLLineRenderer::buildMarkerVertices(const QVector<QVector2D> &points) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  m_markerVertices.clear();

  if (!lineData->marker.visible) {
    return;
  }

  const float radius = lineData->marker.radius;

  for (const QVector2D &center : points) {
    for (int i = 0; i < MarkerSegments; ++i) {
      const float a0 = static_cast<float>(i) /
                       static_cast<float>(MarkerSegments) * 2.0f *
                       static_cast<float>(M_PI);

      const float a1 = static_cast<float>(i + 1) /
                       static_cast<float>(MarkerSegments) * 2.0f *
                       static_cast<float>(M_PI);

      const QVector2D p0 = center;

      const QVector2D p1 =
          center + QVector2D(std::cos(a0) * radius, std::sin(a0) * radius);

      const QVector2D p2 =
          center + QVector2D(std::cos(a1) * radius, std::sin(a1) * radius);

      appendTriangle(m_markerVertices, MarkerVertex{p0}, MarkerVertex{p1},
                     MarkerVertex{p2});
    }
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
  m_markerProgram->setUniformValue("u_radius", lineData->marker.radius);
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
      0, 2, GL_FLOAT, GL_FALSE, sizeof(MarkerVertex),
      reinterpret_cast<void *>(offsetof(MarkerVertex, position)));

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

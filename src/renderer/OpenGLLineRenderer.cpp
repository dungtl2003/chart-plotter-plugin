#include "ChartPlotter/renderer/OpenGLLineRenderer.hpp"
#include "ChartPlotter/utils/Gl.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "ChartPlotter/utils/RenderMath.hpp"
#include <qcolor.h>
#include <qvectornd.h>

namespace ChartPlotter {

namespace {

constexpr float InsideSdf = -1.0f;
constexpr float BoundarySdf = 0.0f;
constexpr int MarkerSegments = 32;

double normalize(double value, double min, double max) {
  const double range = max - min;

  if (std::abs(range) < 1e-12) {
    return 0.5;
  }

  return (value - min) / range;
}

QPointF mapDataToItem(const QPointF &point, const DataRange &xRange,
                      const DataRange &yRange, const QRectF &plotArea) {
  const double nx = normalize(point.x(), xRange.min, xRange.max);
  const double ny = normalize(point.y(), yRange.min, yRange.max);

  const double x = plotArea.left() + nx * plotArea.width();

  // invert y axis
  const double y = plotArea.bottom() - ny * plotArea.height();

  return QPointF(x, y);
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

  if (m_strokeCoreVbo) {
    f->glDeleteBuffers(1, &m_strokeCoreVbo);
    m_strokeCoreVbo = 0;
  }

  if (m_strokeCoreVao) {
    f->glDeleteVertexArrays(1, &m_strokeCoreVao);
    m_strokeCoreVao = 0;
  }

  if (m_strokeFringeVbo) {
    f->glDeleteBuffers(1, &m_strokeFringeVbo);
    m_strokeFringeVbo = 0;
  }

  if (m_strokeFringeVao) {
    f->glDeleteVertexArrays(1, &m_strokeFringeVao);
    m_strokeFringeVao = 0;
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
  const DataRange xRange = context.xRange;
  const DataRange yRange = context.yRange;

  QVector<QVector2D> points;
  points.reserve(m_data.get()->points.size());

  for (const auto &p : m_data.get()->points) {
    points.push_back(
        QVector2D(mapDataToItem(p, xRange, yRange, context.itemRect)));
  }

  // const float rwidth = float(context.itemRect.width());
  // const float rheight = float(context.itemRect.height());
  // QVector<QVector2D> points = {
  //     {rwidth * 0.00f, rheight * 0.65f}, {rwidth * 0.05f, rheight * 0.48f},
  //     {rwidth * 0.10f, rheight * 0.58f}, {rwidth * 0.15f, rheight * 0.36f},
  //     {rwidth * 0.20f, rheight * 0.42f}, {rwidth * 0.25f, rheight * 0.30f},
  //     {rwidth * 0.30f, rheight * 0.52f}, {rwidth * 0.35f, rheight * 0.46f},
  //     {rwidth * 0.40f, rheight * 0.68f}, {rwidth * 0.45f, rheight * 0.55f},
  //     {rwidth * 0.50f, rheight * 0.60f}, {rwidth * 0.55f, rheight * 0.38f},
  //     {rwidth * 0.60f, rheight * 0.44f}, {rwidth * 0.65f, rheight * 0.28f},
  //     {rwidth * 0.70f, rheight * 0.34f}, {rwidth * 0.75f, rheight * 0.22f},
  //     {rwidth * 0.80f, rheight * 0.40f}, {rwidth * 0.85f, rheight * 0.33f},
  //     {rwidth * 0.90f, rheight * 0.18f}, {rwidth * 0.95f, rheight * 0.26f},
  //     {rwidth * 1.00f, rheight * 0.14f},
  // };
  // QVector<QVector2D> points = {
  //     {0.0f, 0.0f},
  //     {rwidth, rheight},
  // };

  buildStrokeVertices(points);
  buildMarkerVertices(points);

  uploadStrokeVertices(f);
  uploadMarkerVertices(f);

  f->glEnable(GL_BLEND);
  f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  bindStrokeProgram(mvp);
  drawStrokeFringesAsTriangles(f);
  m_strokeProgram->release();

  bindStrokeProgram(mvp);
  drawStrokeCoresAsTriangles(f);
  m_strokeProgram->release();

  bindMarkerProgram(mvp);
  drawMarkersAsTriangles(f);
  m_markerProgram->release();

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
  buildStrokeCoreVertices(points);
  buildStrokeFringeVertices(points);
}

void OpenGLLineRenderer::buildStrokeCoreVertices(
    const QVector<QVector2D> &points) {
  m_strokeCoreVertices.clear();
  buildSegmentCoreVertices(points);
  buildMiterJoinCoreVertices(points);
}

void OpenGLLineRenderer::buildSegmentCoreVertices(
    const QVector<QVector2D> &points) {
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

    if (dir.lengthSquared() <= 0.0001f)
      continue;

    dir.normalize();

    QVector2D n(-dir.y(), dir.x());

    StrokeSdfVertex v0{p0 - n * halfWidth, InsideSdf};
    StrokeSdfVertex v1{p0 + n * halfWidth, InsideSdf};
    StrokeSdfVertex v2{p1 - n * halfWidth, InsideSdf};
    StrokeSdfVertex v3{p1 + n * halfWidth, InsideSdf};

    m_strokeCoreVertices.push_back(v0);
    m_strokeCoreVertices.push_back(v1);
    m_strokeCoreVertices.push_back(v2);

    m_strokeCoreVertices.push_back(v2);
    m_strokeCoreVertices.push_back(v1);
    m_strokeCoreVertices.push_back(v3);
  }
}

void OpenGLLineRenderer::buildStrokeFringeVertices(
    const QVector<QVector2D> &points) {
  m_strokeFringeVertices.clear();
  buildSegmentFringeVertices(points);
  buildMiterJoinFringeVertices(points);
}

void OpenGLLineRenderer::buildSegmentFringeVertices(
    const QVector<QVector2D> &points) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  if (points.size() < 2) {
    return;
  }

  const float outerSdf = lineData->antialias;
  const float halfWidth = lineData->stroke.width * 0.5f;
  const float outerWidth = halfWidth + lineData->antialias;

  for (std::size_t i = 0; i + 1 < points.size(); ++i) {
    QVector2D p0 = points[i];
    QVector2D p1 = points[i + 1];

    QVector2D dir = p1 - p0;

    if (dir.lengthSquared() <= 0.0001f) {
      continue;
    }

    dir.normalize();

    QVector2D normal(-dir.y(), dir.x());

    // Positive side fringe
    StrokeSdfVertex a0{p0 + normal * halfWidth, BoundarySdf};
    StrokeSdfVertex a1{p0 + normal * outerWidth, outerSdf};
    StrokeSdfVertex a2{p1 + normal * halfWidth, BoundarySdf};
    StrokeSdfVertex a3{p1 + normal * outerWidth, outerSdf};

    m_strokeFringeVertices.push_back(a0);
    m_strokeFringeVertices.push_back(a1);
    m_strokeFringeVertices.push_back(a2);

    m_strokeFringeVertices.push_back(a2);
    m_strokeFringeVertices.push_back(a1);
    m_strokeFringeVertices.push_back(a3);

    // Negative side fringe
    StrokeSdfVertex b0{p0 - normal * halfWidth, BoundarySdf};
    StrokeSdfVertex b1{p1 - normal * halfWidth, BoundarySdf};
    StrokeSdfVertex b2{p0 - normal * outerWidth, outerSdf};
    StrokeSdfVertex b3{p1 - normal * outerWidth, outerSdf};

    m_strokeFringeVertices.push_back(b0);
    m_strokeFringeVertices.push_back(b2);
    m_strokeFringeVertices.push_back(b1);

    m_strokeFringeVertices.push_back(b1);
    m_strokeFringeVertices.push_back(b2);
    m_strokeFringeVertices.push_back(b3);
  }
}

void OpenGLLineRenderer::buildMiterJoinCoreVertices(
    const QVector<QVector2D> &points) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  constexpr float InsideSdf = -1.0f;

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
    bool useMiter = (miterLength / halfWidth) <= 4.0f;

    if (useMiter) {
      // Full solid wedge.
      m_strokeCoreVertices.push_back({p1, InsideSdf});
      m_strokeCoreVertices.push_back({edgeA, InsideSdf});
      m_strokeCoreVertices.push_back({miterPoint, InsideSdf});

      m_strokeCoreVertices.push_back({p1, InsideSdf});
      m_strokeCoreVertices.push_back({miterPoint, InsideSdf});
      m_strokeCoreVertices.push_back({edgeB, InsideSdf});
    } else {
      // Bevel fallback.
      m_strokeCoreVertices.push_back({p1, InsideSdf});
      m_strokeCoreVertices.push_back({edgeA, InsideSdf});
      m_strokeCoreVertices.push_back({edgeB, InsideSdf});
    }
  }
}

void OpenGLLineRenderer::buildMiterJoinFringeVertices(
    const QVector<QVector2D> &points) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  if (points.size() < 3) {
    return;
  }

  const float outerSdf = lineData->antialias;
  const float halfWidth = lineData->stroke.width * 0.5f;
  const float outerWidth = halfWidth + lineData->antialias;

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

    if (std::abs(turn) <= 0.0001f)
      continue;

    QVector2D nA = RenderMath::perpendicularLeft(dirA);
    QVector2D nB = RenderMath::perpendicularLeft(dirB);

    QVector2D sideA;
    QVector2D sideB;

    if (turn > 0.0f) {
      sideA = -nA;
      sideB = -nB;
    } else {
      sideA = nA;
      sideB = nB;
    }

    QVector2D edgeABoundary = p1 + sideA * halfWidth;
    QVector2D edgeBBoundary = p1 + sideB * halfWidth;

    QVector2D edgeAOuter = p1 + sideA * outerWidth;
    QVector2D edgeBOuter = p1 + sideB * outerWidth;

    QVector2D miterBoundary;
    QVector2D miterOuter;

    bool okBoundary = RenderMath::lineIntersection(
        edgeABoundary, dirA, edgeBBoundary, dirB, miterBoundary);

    bool okOuter = RenderMath::lineIntersection(edgeAOuter, dirA, edgeBOuter,
                                                dirB, miterOuter);

    if (!okBoundary || !okOuter) {
      continue;
    }

    float miterLength = (miterBoundary - p1).length();
    bool useMiter = (miterLength / halfWidth) <= 4.0f;

    if (!useMiter) {
      m_strokeFringeVertices.push_back({edgeABoundary, BoundarySdf});
      m_strokeFringeVertices.push_back({edgeAOuter, outerSdf});
      m_strokeFringeVertices.push_back({edgeBBoundary, BoundarySdf});

      m_strokeFringeVertices.push_back({edgeBBoundary, BoundarySdf});
      m_strokeFringeVertices.push_back({edgeAOuter, outerSdf});
      m_strokeFringeVertices.push_back({edgeBOuter, outerSdf});
      continue;
    }

    // Miter outer fringe band.
    StrokeSdfVertex ai{edgeABoundary, BoundarySdf};
    StrokeSdfVertex ao{edgeAOuter, outerSdf};

    StrokeSdfVertex mi{miterBoundary, BoundarySdf};
    StrokeSdfVertex mo{miterOuter, outerSdf};

    StrokeSdfVertex bi{edgeBBoundary, BoundarySdf};
    StrokeSdfVertex bo{edgeBOuter, outerSdf};

    m_strokeFringeVertices.push_back(ai);
    m_strokeFringeVertices.push_back(ao);
    m_strokeFringeVertices.push_back(mi);

    m_strokeFringeVertices.push_back(mi);
    m_strokeFringeVertices.push_back(ao);
    m_strokeFringeVertices.push_back(mo);

    m_strokeFringeVertices.push_back(mi);
    m_strokeFringeVertices.push_back(mo);
    m_strokeFringeVertices.push_back(bi);

    m_strokeFringeVertices.push_back(bi);
    m_strokeFringeVertices.push_back(mo);
    m_strokeFringeVertices.push_back(bo);
  }
}

void OpenGLLineRenderer::buildMarkerVertices(const QVector<QVector2D> &points) {
  assert(m_data != nullptr);
  const auto &lineData = m_data.get();

  m_markerVertices.clear();

  const float outerRadius = lineData->marker.radius + lineData->marker.radius;

  for (const QVector2D &center : points) {
    const QVector2D p0 = center + QVector2D(-outerRadius, -outerRadius);
    const QVector2D p1 = center + QVector2D(+outerRadius, -outerRadius);
    const QVector2D p2 = center + QVector2D(-outerRadius, +outerRadius);
    const QVector2D p3 = center + QVector2D(+outerRadius, +outerRadius);

    m_markerVertices.push_back({p0, center});
    m_markerVertices.push_back({p2, center});
    m_markerVertices.push_back({p1, center});

    m_markerVertices.push_back({p1, center});
    m_markerVertices.push_back({p2, center});
    m_markerVertices.push_back({p3, center});
  }
}

void OpenGLLineRenderer::uploadStrokeVertices(QOpenGLExtraFunctions *f) {
  uploadStrokeCoreVertices(f);
  uploadStrokeFringeVertices(f);
}

void OpenGLLineRenderer::uploadStrokeCoreVertices(QOpenGLExtraFunctions *f) {
  f->glBindBuffer(GL_ARRAY_BUFFER, m_strokeCoreVbo);

  f->glBufferData(GL_ARRAY_BUFFER,
                  static_cast<GLsizeiptr>(m_strokeCoreVertices.size() *
                                          sizeof(StrokeSdfVertex)),
                  m_strokeCoreVertices.data(), GL_DYNAMIC_DRAW);

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLLineRenderer::uploadStrokeFringeVertices(QOpenGLExtraFunctions *f) {
  f->glBindBuffer(GL_ARRAY_BUFFER, m_strokeFringeVbo);

  f->glBufferData(GL_ARRAY_BUFFER,
                  static_cast<GLsizeiptr>(m_strokeFringeVertices.size() *
                                          sizeof(StrokeSdfVertex)),
                  m_strokeFringeVertices.data(), GL_DYNAMIC_DRAW);

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

void OpenGLLineRenderer::drawStrokeCoresAsTriangles(QOpenGLExtraFunctions *f) {
  if (m_strokeCoreVertices.empty()) {
    return;
  }

  f->glBindVertexArray(m_strokeCoreVao);
  f->glDrawArrays(GL_TRIANGLES, 0,
                  static_cast<GLsizei>(m_strokeCoreVertices.size()));
  f->glBindVertexArray(0);
}
void OpenGLLineRenderer::drawStrokeFringesAsTriangles(
    QOpenGLExtraFunctions *f) {
  if (m_strokeFringeVertices.empty()) {
    return;
  }

  f->glBindVertexArray(m_strokeFringeVao);
  f->glDrawArrays(GL_TRIANGLES, 0,
                  static_cast<GLsizei>(m_strokeFringeVertices.size()));
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
  initializeStrokeCoreGeometry(f);
  initializeStrokeFringeGeometry(f);
}

void OpenGLLineRenderer::initializeStrokeCoreGeometry(
    QOpenGLExtraFunctions *f) {
  f->glGenVertexArrays(1, &m_strokeCoreVao);
  f->glGenBuffers(1, &m_strokeCoreVbo);

  f->glBindVertexArray(m_strokeCoreVao);
  f->glBindBuffer(GL_ARRAY_BUFFER, m_strokeCoreVbo);

  f->glEnableVertexAttribArray(0);
  f->glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, sizeof(StrokeSdfVertex),
      reinterpret_cast<void *>(offsetof(StrokeSdfVertex, position)));

  f->glEnableVertexAttribArray(1);
  f->glVertexAttribPointer(
      1, 1, GL_FLOAT, GL_FALSE, sizeof(StrokeSdfVertex),
      reinterpret_cast<void *>(offsetof(StrokeSdfVertex, sdf)));

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
  f->glBindVertexArray(0);
}

void OpenGLLineRenderer::initializeStrokeFringeGeometry(
    QOpenGLExtraFunctions *f) {
  f->glGenVertexArrays(1, &m_strokeFringeVao);
  f->glGenBuffers(1, &m_strokeFringeVbo);

  f->glBindVertexArray(m_strokeFringeVao);
  f->glBindBuffer(GL_ARRAY_BUFFER, m_strokeFringeVbo);

  f->glEnableVertexAttribArray(0);
  f->glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, sizeof(StrokeSdfVertex),
      reinterpret_cast<void *>(offsetof(StrokeSdfVertex, position)));

  f->glEnableVertexAttribArray(1);
  f->glVertexAttribPointer(
      1, 1, GL_FLOAT, GL_FALSE, sizeof(StrokeSdfVertex),
      reinterpret_cast<void *>(offsetof(StrokeSdfVertex, sdf)));

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

  const QString strokeVertexShader = Gl::readShaderSource(
      ":/qt/qml/ChartPlotter/shaders/line/stroke-sdf.vert");
  const QString strokeFragmentShader = Gl::readShaderSource(
      ":/qt/qml/ChartPlotter/shaders/line/stroke-sdf.frag");
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

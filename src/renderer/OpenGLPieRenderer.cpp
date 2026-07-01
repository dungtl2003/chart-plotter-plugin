#include "ChartPlotter/renderer/OpenGLPieRenderer.hpp"
#include "ChartPlotter/utils/Gl.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

#include <QtMath>

#include <algorithm>
#include <cmath>

namespace ChartPlotter {

namespace {

// Fraction of the available half-extent the pie radius fills, leaving a small
// margin so the circle never touches the plot edge.
constexpr double kRadiusFill = 0.9;
// Angular resolution of the arc tessellation (radians ~= 2 degrees per step).
constexpr double kSegmentAngle = M_PI / 90.0;

// Clockwise from the top (12 o'clock): a=0 -> up, a=pi/2 -> right. Screen y
// grows downward, hence the -cos on y.
QPointF pointOnCircle(const QPointF &center, double radius, double angle) {
  return QPointF(center.x() + radius * std::sin(angle),
                 center.y() - radius * std::cos(angle));
}

} // namespace

void OpenGLPieRenderer::initialize(QOpenGLExtraFunctions *f) {
  CP_DEBUG("OpenGLPieRenderer::initialize: Initing OpenGL resources...");

  initializeProgram();
  initializeGeometry(f);
}

void OpenGLPieRenderer::release(QOpenGLExtraFunctions *f) {
  CP_DEBUG("OpenGLPieRenderer::release: Releasing OpenGL resources...");

  if (m_vbo) {
    f->glDeleteBuffers(1, &m_vbo);
    m_vbo = 0;
  }
  if (m_vao) {
    f->glDeleteVertexArrays(1, &m_vao);
    m_vao = 0;
  }

  m_program.reset();
}

void OpenGLPieRenderer::render(const ChartRenderContext &context) {
  if (!m_program || !m_data) {
    return;
  }

  buildVertices(context);

  if (m_vertices.empty()) {
    return;
  }

  QOpenGLExtraFunctions *f = context.f;

  f->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  f->glBufferData(
      GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(m_vertices.size() * sizeof(PieVertex)),
      m_vertices.data(), GL_DYNAMIC_DRAW);
  f->glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_program->bind();
  m_program->setUniformValue("u_mvp", context.mvp);

  f->glBindVertexArray(m_vao);
  f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
  f->glBindVertexArray(0);

  m_program->release();
}

void OpenGLPieRenderer::setData(std::unique_ptr<RenderData> data) {
  if (PieRenderData *pieData = dynamic_cast<PieRenderData *>(data.get())) {
    auto _ = data.release(); // stop tracking so we can take ownership
    m_data = std::unique_ptr<PieRenderData>(pieData);
  } else {
    m_data = nullptr;
  }
}

void OpenGLPieRenderer::buildVertices(const ChartRenderContext &context) {
  m_vertices.clear();

  if (!m_data || m_data->slices.isEmpty()) {
    return;
  }

  const QRectF &plotArea = context.plotArea;
  const QPointF center = plotArea.center();
  const double radius =
      0.5 * std::min(plotArea.width(), plotArea.height()) * kRadiusFill;

  if (radius <= 0.0) {
    return;
  }

  // Rough upper bound: each slice is a fan of segments, one triangle each.
  m_vertices.reserve((static_cast<int>(2.0 * M_PI / kSegmentAngle) + 4) * 3);

  const QVector2D c(static_cast<float>(center.x()),
                    static_cast<float>(center.y()));

  for (const PieSlice &slice : m_data->slices) {
    if (slice.sweepAngle <= 0.0) {
      continue;
    }

    const QVector4D color(slice.color.redF(), slice.color.greenF(),
                          slice.color.blueF(), slice.color.alphaF());

    const int segments =
        std::max(1, static_cast<int>(std::ceil(slice.sweepAngle /
                                               kSegmentAngle)));
    const double step = slice.sweepAngle / segments;

    for (int s = 0; s < segments; ++s) {
      const double a0 = slice.startAngle + step * s;
      const double a1 = slice.startAngle + step * (s + 1);

      const QPointF p0 = pointOnCircle(center, radius, a0);
      const QPointF p1 = pointOnCircle(center, radius, a1);

      m_vertices.push_back(PieVertex{c, color});
      m_vertices.push_back(
          PieVertex{QVector2D(static_cast<float>(p0.x()),
                              static_cast<float>(p0.y())),
                    color});
      m_vertices.push_back(
          PieVertex{QVector2D(static_cast<float>(p1.x()),
                              static_cast<float>(p1.y())),
                    color});
    }
  }
}

void OpenGLPieRenderer::initializeProgram() {
  const QString vertexShader =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/pie/pie.vert");
  const QString fragmentShader =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/pie/pie.frag");

  m_program = Gl::createProgram(vertexShader, fragmentShader, "PieProgram");
  if (!m_program) {
    CP_WARN("OpenGLPieRenderer::initialize: failed to create shader program");
  }
}

void OpenGLPieRenderer::initializeGeometry(QOpenGLExtraFunctions *f) {
  f->glGenVertexArrays(1, &m_vao);
  f->glGenBuffers(1, &m_vbo);

  f->glBindVertexArray(m_vao);
  f->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

  f->glEnableVertexAttribArray(0);
  f->glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, sizeof(PieVertex),
      reinterpret_cast<void *>(offsetof(PieVertex, position)));

  f->glEnableVertexAttribArray(1);
  f->glVertexAttribPointer(
      1, 4, GL_FLOAT, GL_FALSE, sizeof(PieVertex),
      reinterpret_cast<void *>(offsetof(PieVertex, color)));

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
  f->glBindVertexArray(0);
}

} // namespace ChartPlotter

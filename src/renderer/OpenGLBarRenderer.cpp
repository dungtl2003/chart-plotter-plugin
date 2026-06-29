#include "ChartPlotter/renderer/OpenGLBarRenderer.hpp"
#include "ChartPlotter/constants/ChartConstants.hpp"
#include "ChartPlotter/utils/Gl.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "ChartPlotter/utils/RenderMath.hpp"

#include <algorithm>

namespace ChartPlotter {

namespace {

QPointF mapDataToItem(double dataX, double dataY, const AxisRange &xRange,
                      const AxisRange &yRange, const QRectF &plotArea) {
  const double nx = RenderMath::normalize(dataX, xRange.min, xRange.max);
  const double ny = RenderMath::normalize(dataY, yRange.min, yRange.max);

  const double x = plotArea.left() + nx * plotArea.width();
  // invert y axis (screen y grows downward)
  const double y = plotArea.bottom() - ny * plotArea.height();

  return QPointF(x, y);
}

} // namespace

void OpenGLBarRenderer::initialize(QOpenGLExtraFunctions *f) {
  CP_DEBUG("OpenGLBarRenderer::initialize: Initing OpenGL resources...");

  initializeProgram();
  initializeGeometry(f);
}

void OpenGLBarRenderer::release(QOpenGLExtraFunctions *f) {
  CP_DEBUG("OpenGLBarRenderer::release: Releasing OpenGL resources...");

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

void OpenGLBarRenderer::render(const ChartRenderContext &context) {
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
      static_cast<GLsizeiptr>(m_vertices.size() * sizeof(BarVertex)),
      m_vertices.data(), GL_DYNAMIC_DRAW);
  f->glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_program->bind();
  m_program->setUniformValue("u_mvp", context.mvp);
  m_program->setUniformValue("u_color", m_data->color);
  // vec4(left, right, top, bottom)
  m_program->setUniformValue(
      "u_plotArea",
      QVector4D(context.plotArea.left(), context.plotArea.right(),
                context.plotArea.top(), context.plotArea.bottom()));

  f->glBindVertexArray(m_vao);
  f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
  f->glBindVertexArray(0);

  m_program->release();
}

void OpenGLBarRenderer::setData(std::unique_ptr<RenderData> data) {
  if (BarRenderData *barData = dynamic_cast<BarRenderData *>(data.get())) {
    auto _ = data.release(); // stop tracking so we can take ownership
    m_data = std::unique_ptr<BarRenderData>(barData);
  } else {
    m_data = nullptr;
  }
}

void OpenGLBarRenderer::buildVertices(const ChartRenderContext &context) {
  m_vertices.clear();

  if (!m_data || m_data->points.isEmpty()) {
    return;
  }

  const AxisRange xRange = context.xAxisRange;
  const AxisRange yRange = context.yAxisRange;
  const QRectF &plotArea = context.plotArea;

  // Baseline (data y == baseline) projected to screen space.
  const double baselineY =
      mapDataToItem(0.0, m_data->baseline, xRange, yRange, plotArea).y();
  const float botY = static_cast<float>(baselineY);

  // Auto-size each bar to its band. The band is a fixed width in data units;
  // we map it to pixels against the current axis range, so bars widen as you
  // zoom in and shrink to fit when there are many of them.
  const double xSpan = static_cast<double>(xRange.max) - xRange.min;
  float effectiveWidth = ChartConstants::BAR_MIN_PIXEL_WIDTH;
  if (xSpan > RenderMath::Epsilon) {
    const double bandPx = m_data->bandWidth / xSpan * plotArea.width();
    effectiveWidth =
        std::max(static_cast<float>(bandPx) * ChartConstants::BAR_BAND_FILL,
                 ChartConstants::BAR_MIN_PIXEL_WIDTH);
  }

  const float halfWidth = effectiveWidth * 0.5f;

  m_vertices.reserve(m_data->points.size() * 6);

  for (const QPointF &p : m_data->points) {
    const QPointF top = mapDataToItem(p.x(), p.y(), xRange, yRange, plotArea);
    const float cx = static_cast<float>(top.x());
    const float left = cx - halfWidth;
    const float right = cx + halfWidth;
    const float topY = static_cast<float>(top.y());

    const BarVertex v0{QVector2D(left, botY)};
    const BarVertex v1{QVector2D(right, botY)};
    const BarVertex v2{QVector2D(left, topY)};
    const BarVertex v3{QVector2D(right, topY)};

    RenderMath::appendQuad(m_vertices, v0, v1, v2, v3);
  }
}

void OpenGLBarRenderer::initializeProgram() {
  const QString vertexShader =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/bar/bar.vert");
  const QString fragmentShader =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/bar/bar.frag");

  m_program = Gl::createProgram(vertexShader, fragmentShader, "BarProgram");
  if (!m_program) {
    CP_WARN("OpenGLBarRenderer::initialize: failed to create shader program");
  }
}

void OpenGLBarRenderer::initializeGeometry(QOpenGLExtraFunctions *f) {
  f->glGenVertexArrays(1, &m_vao);
  f->glGenBuffers(1, &m_vbo);

  f->glBindVertexArray(m_vao);
  f->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

  f->glEnableVertexAttribArray(0);
  f->glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, sizeof(BarVertex),
      reinterpret_cast<void *>(offsetof(BarVertex, position)));

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
  f->glBindVertexArray(0);
}

} // namespace ChartPlotter

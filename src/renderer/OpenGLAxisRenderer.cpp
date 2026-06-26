#include "ChartPlotter/renderer/OpenGLAxisRenderer.hpp"
#include "ChartPlotter/utils/Gl.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "ChartPlotter/utils/RenderMath.hpp"

#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>

namespace ChartPlotter {

namespace {

float calculateTickLength(float axisWidth) { return axisWidth * 4.0f; }

void appendStrokeQuad(QVector<AxisStrokeVertex> &outVertices,
                      const QVector2D &p0, const QVector2D &p1,
                      float halfWidth) {
  QVector2D dir = p1 - p0;

  if (dir.lengthSquared() <= 0.0001f) {
    return;
  }

  dir.normalize();

  const QVector2D n(-dir.y(), dir.x());

  const AxisStrokeVertex v0{p0 - n * halfWidth};
  const AxisStrokeVertex v1{p0 + n * halfWidth};
  const AxisStrokeVertex v2{p1 - n * halfWidth};
  const AxisStrokeVertex v3{p1 + n * halfWidth};

  RenderMath::appendQuad(outVertices, v0, v1, v2, v3);
}

void appendTexturedQuad(QVector<TextVertex> &out, const QVector2D &topLeft,
                        const QVector2D &size) {
  const float x0 = topLeft.x();
  const float y0 = topLeft.y(); // top
  const float x1 = x0 + size.x();
  const float y1 = y0 + size.y(); // bottom
  out.push_back(TextVertex{{x0, y0}, {0.f, 0.f}});
  out.push_back(TextVertex{{x1, y0}, {1.f, 0.f}});
  out.push_back(TextVertex{{x0, y1}, {0.f, 1.f}});
  out.push_back(TextVertex{{x1, y0}, {1.f, 0.f}});
  out.push_back(TextVertex{{x1, y1}, {1.f, 1.f}});
  out.push_back(TextVertex{{x0, y1}, {0.f, 1.f}});
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

} // namespace

void OpenGLAxisRenderer::initialize(QOpenGLExtraFunctions *f) {
  CP_DEBUG("OpenGLAxisRenderer::initialize: Initing OpenGL resources...");

  initializePrograms();
  initializeStrokeGeometry(f);
  initializeTextGeometry(f);
}

void OpenGLAxisRenderer::release(QOpenGLExtraFunctions *f) {
  CP_DEBUG("OpenGLAxisRenderer::release: Releasing OpenGL resources...");

  if (m_strokeVbo) {
    f->glDeleteBuffers(1, &m_strokeVbo);
    m_strokeVbo = 0;
  }
  if (m_strokeVao) {
    f->glDeleteVertexArrays(1, &m_strokeVao);
    m_strokeVao = 0;
  }
  if (m_textVbo) {
    f->glDeleteBuffers(1, &m_textVbo);
    m_textVbo = 0;
  }
  if (m_textVao) {
    f->glDeleteVertexArrays(1, &m_textVao);
    m_textVao = 0;
  }

  for (const auto &lt : m_labelTextureCache) {
    f->glDeleteTextures(1, &lt.id);
  }
  m_labelTextureCache.clear();

  m_textProgram.reset();
  m_strokeProgram.reset();
}

void OpenGLAxisRenderer::render(const ChartRenderContext &context) {
  if (!m_strokeProgram || !m_data) {
    return;
  }

  AxisRenderData *axisData = m_data.get();
  QOpenGLExtraFunctions *f = context.f;
  const QMatrix4x4 &mvp = context.mvp;

  const double halfWidth = axisData->width * 0.5f;
  const AxisRange xRange = context.xAxisRange;
  const AxisRange yRange = context.yAxisRange;

  QVector<Tick> ticks;
  ticks.reserve(axisData->ticks.size());
  for (const auto &tick : axisData->ticks) {
    ticks.push_back(Tick{.position = QVector2D(mapDataToItem(
                             tick.pos, xRange, yRange, context.plotArea)),
                         .label = tick.label});
  }

  const float dpr = static_cast<float>(qGuiApp->devicePixelRatio());

  buildAxisAndTickVertices(context, ticks);
  // buildGridVertices(context, ticks);
  buildLabelVertices(context, ticks, f, dpr);

  // uploadStrokeVertices(f, m_gridVertices);
  // bindStrokeProgram(mvp, axisData->axisColor.lighter(125));
  // drawStrokesAsTriangles(f, m_gridVertices);

  uploadStrokeVertices(f, m_axisAndTickVertices);
  bindStrokeProgram(mvp, axisData->axisColor);
  drawStrokesAsTriangles(f, m_axisAndTickVertices);

  drawLabels(f, mvp, axisData->fontColor);

  m_strokeProgram->release();
}

void OpenGLAxisRenderer::setData(std::unique_ptr<RenderData> data) {
  if (AxisRenderData *axisData = dynamic_cast<AxisRenderData *>(data.get())) {
    auto _ =
        data.release(); // so it does not track and auto remove the data inside
    m_data = std::unique_ptr<AxisRenderData>(axisData);
  }
};

OpenGLAxisRenderer::LabelTexture
OpenGLAxisRenderer::rasterizeLabel(const QString &text,
                                   QOpenGLExtraFunctions *f, float dpr) {
  const QFontMetricsF fm(m_labelFont);
  const float wLogical =
      static_cast<float>(std::ceil(fm.horizontalAdvance(text)));
  const float hLogical = static_cast<float>(std::ceil(fm.height()));
  const int wPx = std::max(1, static_cast<int>(std::ceil(wLogical * dpr)));
  const int hPx = std::max(1, static_cast<int>(std::ceil(hLogical * dpr)));

  QImage img(wPx, hPx, QImage::Format_RGBA8888_Premultiplied);
  img.setDevicePixelRatio(dpr); // lets us paint in logical coords
  img.fill(Qt::transparent);
  {
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setFont(m_labelFont);
    p.setPen(Qt::white); // coverage lands in the alpha channel
    p.drawText(QRectF(0, 0, wLogical, hLogical),
               Qt::AlignLeft | Qt::AlignVCenter, text);
  }

  GLuint tex = 0;
  f->glGenTextures(1, &tex);
  f->glBindTexture(GL_TEXTURE_2D, tex);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0,
                  GL_RGBA, GL_UNSIGNED_BYTE, img.constBits());
  f->glBindTexture(GL_TEXTURE_2D, 0);

  return LabelTexture{tex, wLogical, hLogical}; // quad uses logical size
}

OpenGLAxisRenderer::LabelTexture
OpenGLAxisRenderer::acquireLabelTexture(const QString &text,
                                        QOpenGLExtraFunctions *f, float dpr) {
  const auto it = m_labelTextureCache.constFind(text);
  if (it != m_labelTextureCache.constEnd())
    return it.value();
  const LabelTexture lt = rasterizeLabel(text, f, dpr);
  m_labelTextureCache.insert(text, lt);
  return lt;
}

auto OpenGLAxisRenderer::resolveTickAnchors(const QVector<Tick> &ticks) const
    -> QVector<Tick> {
  assert(m_data != nullptr);
  const auto *axisData = m_data.get();

  if (axisData->tickMode == ChartEnums::TickMode::OnTick) {
    return ticks;
  }

  // Category / bar axes: incoming positions are band boundaries; the tick
  // (mark + label) is centered in each gap. tick[i].label labels the gap
  // between tick[i] and tick[i+1]; the last tick contributes only its position.
  QVector<Tick> anchors;
  if (ticks.size() < 2) {
    return anchors;
  }

  anchors.reserve(ticks.size() - 1);
  for (qsizetype i = 0; i + 1 < ticks.size(); ++i) {
    const QVector2D mid =
        (ticks.at(i).position + ticks.at(i + 1).position) * 0.5f;
    anchors.push_back(Tick{.position = mid, .label = ticks.at(i).label});
  }
  return anchors;
}

void OpenGLAxisRenderer::buildAxisAndTickVertices(
    const ChartRenderContext &context, const QVector<Tick> &ticks) {
  assert(m_data != nullptr);
  const auto &axisData = m_data.get();

  m_axisAndTickVertices.clear();

  const AxisRange xRange = context.xAxisRange;
  const AxisRange yRange = context.yAxisRange;
  QVector2D firstPoint = QVector2D(mapDataToItem(
      axisData->minPointInRange, xRange, yRange, context.plotArea));
  QVector2D lastPoint = QVector2D(mapDataToItem(
      axisData->maxPointInRange, xRange, yRange, context.plotArea));

  buildAxisVertices(m_axisAndTickVertices, firstPoint, lastPoint);
  buildTickVertices(m_axisAndTickVertices, ticks);
}

void OpenGLAxisRenderer::buildAxisVertices(QVector<AxisStrokeVertex> &out,
                                           const QVector2D &firstPoint,
                                           const QVector2D &lastPoint) {
  assert(m_data != nullptr);
  const auto &axisData = m_data.get();

  const float halfWidth = axisData->width * 0.5f;
  appendStrokeQuad(out, firstPoint, lastPoint, halfWidth);
}

void OpenGLAxisRenderer::buildTickVertices(QVector<AxisStrokeVertex> &out,
                                           const QVector<Tick> &ticks) {
  assert(m_data != nullptr);
  const auto &axisData = m_data.get();

  const float halfWidth = axisData->width * 0.5f;
  const float tickLength = calculateTickLength(axisData->width);
  const float tickHalfLength = tickLength * 0.5f;

  QVector2D a;
  QVector2D b;
  for (const auto &anchor : ticks) {
    const auto &tickPos = anchor.position;
    if (axisData->pos == ChartEnums::AxisPosition::Bottom ||
        axisData->pos == ChartEnums::AxisPosition::Top) {
      a = QVector2D(tickPos.x(), tickPos.y() + tickHalfLength);
      b = QVector2D(tickPos.x(), tickPos.y() - tickHalfLength);
    } else {
      a = QVector2D(tickPos.x() - tickHalfLength, tickPos.y());
      b = QVector2D(tickPos.x() + tickHalfLength, tickPos.y());
    }
    appendStrokeQuad(out, a, b, halfWidth);
  }
}

void OpenGLAxisRenderer::buildGridVertices(const ChartRenderContext &context,
                                           const QVector<Tick> &ticks) {
  assert(m_data != nullptr);
  assert(ticks.size() > 1);

  const auto *axisData = m_data.get();
  const float halfWidth = axisData->width * 0.5f;
  const bool isVertical = axisData->pos == ChartEnums::AxisPosition::Bottom ||
                          axisData->pos == ChartEnums::AxisPosition::Top;

  const double oppositeCoord = [&]() -> double {
    switch (axisData->pos) {
    case ChartEnums::AxisPosition::Bottom:
      return context.plotArea.top() - halfWidth;
    case ChartEnums::AxisPosition::Top:
      return context.plotArea.bottom() + halfWidth;
    case ChartEnums::AxisPosition::Left:
      return context.plotArea.right() + halfWidth;
    case ChartEnums::AxisPosition::Right:
      return context.plotArea.left() - halfWidth;
    default:
      return 0.0;
    }
  }();

  const auto emitLine = [&](const auto &p) {
    const QVector2D a(p.x(), p.y());
    const QVector2D b = isVertical ? QVector2D(p.x(), oppositeCoord)
                                   : QVector2D(oppositeCoord, p.y());
    appendStrokeQuad(m_gridVertices, a, b, halfWidth);
  };

  m_gridVertices.clear();
  constexpr qsizetype kVertsPerStrokeQuad = 6;
  m_gridVertices.reserve(ticks.size() * kVertsPerStrokeQuad);

  for (qsizetype i = 1; i < ticks.size() - 1; ++i) {
    emitLine(ticks.at(i).position);
  }

  const auto &firstPos = axisData->minPointInRange;
  const auto &lastPos = axisData->maxPointInRange;
  if (isVertical) {
    if (!(context.axisPositions & ChartEnums::AxisPosition::Left))
      emitLine(firstPos);
    if (!(context.axisPositions & ChartEnums::AxisPosition::Right))
      emitLine(lastPos);
  } else {
    if (!(context.axisPositions & ChartEnums::AxisPosition::Top))
      emitLine(lastPos);
    if (!(context.axisPositions & ChartEnums::AxisPosition::Bottom))
      emitLine(firstPos);
  }
}

void OpenGLAxisRenderer::buildLabelVertices(const ChartRenderContext &context,
                                            const QVector<Tick> &ticks,
                                            QOpenGLExtraFunctions *f,
                                            float dpr) {
  assert(m_data != nullptr);
  const auto *axisData = m_data.get();

  m_labelVertices.clear();
  m_labelBatches.clear();

  m_labelFont.setPixelSize(axisData->fontSize);
  const float tickHalfLength = calculateTickLength(axisData->width) * 0.5f;
  const float labelGap = 4.0f; // px between tick tip and label
  const float outward = tickHalfLength + labelGap;
  const QVector<Tick> anchors = resolveTickAnchors(ticks);

  for (const auto &tick : anchors) {
    if (tick.label.isEmpty())
      continue;

    const LabelTexture lt = acquireLabelTexture(tick.label, f, dpr);
    const QVector2D &tp = tick.position;
    QVector2D topLeft;

    switch (axisData->pos) {
    case ChartEnums::AxisPosition::Bottom: // below, h-centered
      topLeft = {tp.x() - lt.width * 0.5f, tp.y() + outward};
      break;
    case ChartEnums::AxisPosition::Top: // above, h-centered
      topLeft = {tp.x() - lt.width * 0.5f, tp.y() - outward - lt.height};
      break;
    case ChartEnums::AxisPosition::Left: // left, right-aligned, v-centered
      topLeft = {tp.x() - outward - lt.width, tp.y() - lt.height * 0.5f};
      break;
    case ChartEnums::AxisPosition::Right: // right, left-aligned, v-centered
      topLeft = {tp.x() + outward, tp.y() - lt.height * 0.5f};
      break;
    default:
      continue;
    }

    const int firstVertex = static_cast<int>(m_labelVertices.size());
    appendTexturedQuad(m_labelVertices, topLeft, {lt.width, lt.height});
    m_labelBatches.push_back(LabelBatch{lt.id, firstVertex, 6});
  }
}

void OpenGLAxisRenderer::uploadStrokeVertices(
    QOpenGLExtraFunctions *f, const QVector<AxisStrokeVertex> &vertices) {
  f->glBindBuffer(GL_ARRAY_BUFFER, m_strokeVbo);

  f->glBufferData(
      GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(vertices.size() * sizeof(AxisStrokeVertex)),
      vertices.data(), GL_DYNAMIC_DRAW);

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLAxisRenderer::drawStrokesAsTriangles(
    QOpenGLExtraFunctions *f, const QVector<AxisStrokeVertex> &vertices) {
  if (vertices.empty()) {
    return;
  }

  f->glBindVertexArray(m_strokeVao);
  f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
  f->glBindVertexArray(0);
}

void OpenGLAxisRenderer::drawLabels(QOpenGLExtraFunctions *f,
                                    const QMatrix4x4 &mvp,
                                    const QColor &color) {
  if (m_labelVertices.isEmpty() || !m_textProgram) {
    return;
  }

  f->glBindBuffer(GL_ARRAY_BUFFER, m_textVbo);
  f->glBufferData(
      GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(m_labelVertices.size() * sizeof(TextVertex)),
      m_labelVertices.data(), GL_DYNAMIC_DRAW);
  f->glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_textProgram->bind();
  m_textProgram->setUniformValue("u_mvp", mvp);
  m_textProgram->setUniformValue("u_color", color);
  m_textProgram->setUniformValue("u_texture", 0);

  // const GLboolean blendWasOn = f->glIsEnabled(GL_BLEND);
  // f->glEnable(GL_BLEND);
  // f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  f->glActiveTexture(GL_TEXTURE0);
  f->glBindVertexArray(m_textVao);
  for (const auto &batch : m_labelBatches) {
    f->glBindTexture(GL_TEXTURE_2D, batch.texture);
    f->glDrawArrays(GL_TRIANGLES, batch.firstVertex, batch.vertexCount);
  }
  f->glBindVertexArray(0);
  f->glBindTexture(GL_TEXTURE_2D, 0);

  // if (!blendWasOn) {
  //   f->glDisable(GL_BLEND);
  // }
  m_textProgram->release();
}

void OpenGLAxisRenderer::bindStrokeProgram(const QMatrix4x4 &mvp,
                                           const QColor &color) {
  assert(m_data != nullptr);
  const auto &axisData = m_data.get();

  m_strokeProgram->bind();
  m_strokeProgram->setUniformValue("u_mvp", mvp);
  m_strokeProgram->setUniformValue("u_color", color);
}

void OpenGLAxisRenderer::initializeStrokeGeometry(QOpenGLExtraFunctions *f) {
  f->glGenVertexArrays(1, &m_strokeVao);
  f->glGenBuffers(1, &m_strokeVbo);

  f->glBindVertexArray(m_strokeVao);
  f->glBindBuffer(GL_ARRAY_BUFFER, m_strokeVbo);

  f->glEnableVertexAttribArray(0);
  f->glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, sizeof(AxisStrokeVertex),
      reinterpret_cast<void *>(offsetof(AxisStrokeVertex, position)));

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
  f->glBindVertexArray(0);
}

void OpenGLAxisRenderer::initializeTextGeometry(QOpenGLExtraFunctions *f) {
  f->glGenVertexArrays(1, &m_textVao);
  f->glGenBuffers(1, &m_textVbo);
  f->glBindVertexArray(m_textVao);
  f->glBindBuffer(GL_ARRAY_BUFFER, m_textVbo);
  f->glEnableVertexAttribArray(0);
  f->glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
      reinterpret_cast<void *>(offsetof(TextVertex, position)));
  f->glEnableVertexAttribArray(1);
  f->glVertexAttribPointer(
      1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
      reinterpret_cast<void *>(offsetof(TextVertex, texCoord)));
  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
  f->glBindVertexArray(0);
}

void OpenGLAxisRenderer::initializePrograms() {
  m_strokeProgram = std::make_unique<QOpenGLShaderProgram>();

  const QString strokeVertexShader =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/axis/line.vert");
  const QString strokeFragmentShader =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/axis/line.frag");
  const QString vs =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/axis/text.vert");
  const QString fs =
      Gl::readShaderSource(":/qt/qml/ChartPlotter/shaders/axis/text.frag");

  m_strokeProgram = Gl::createProgram(strokeVertexShader, strokeFragmentShader,
                                      "StrokeProgram");
  m_textProgram = Gl::createProgram(vs, fs, "TextProgram");

  if (!m_strokeProgram || !m_textProgram) {
    CP_WARN("OpenGLAxisRenderer::initialize: failed to create shader "
            "programs");
    return;
  }
}

} // namespace ChartPlotter

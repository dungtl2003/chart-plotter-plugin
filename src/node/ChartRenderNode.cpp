#include "ChartPlotter/node/ChartRenderNode.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "ChartRenderContext.hpp"

#include <QOpenGLContext>

namespace ChartPlotter {

ChartRenderNode::ChartRenderNode() = default;

ChartRenderNode::~ChartRenderNode() {
  releaseResources();
  releaseDebugLogger();
}

QSGRenderNode::StateFlags ChartRenderNode::changedStates() const {
  return ViewportState | ScissorState | ColorState | BlendState;
}

void ChartRenderNode::prepare() {}

void ChartRenderNode::render(const RenderState *state) {
  auto *context = QOpenGLContext::currentContext();

  if (!context || m_renderers.empty()) {
    return;
  }

  initDebugLogger(context);

  auto *f = context->extraFunctions();

  if (!m_initialized) {
    initializeRenderers(f);
    m_initialized = true;
  }

  QMatrix4x4 mvp;

  if (state->projectionMatrix() && matrix()) {
    mvp = *state->projectionMatrix() * *matrix();
  }

  renderRenderers(ChartRenderContext{
      .f = f,
      .mvp = std::move(mvp),
      .itemRect = m_plotContext.itemRect,
      .plotArea = m_plotContext.plotArea,
      .xRange = m_plotContext.xRange,
      .yRange = m_plotContext.yRange,
  });
}

void ChartRenderNode::releaseResources() {
  auto *context = QOpenGLContext::currentContext();
  auto *f = context ? context->extraFunctions() : nullptr;

  if (!m_renderers.empty() && f && m_initialized) {
    releaseRenderers(f);
  }

  m_initialized = false;
}

QSGRenderNode::RenderingFlags ChartRenderNode::flags() const {
  return BoundedRectRendering;
}

void ChartRenderNode::setRenderers(
    std::vector<std::unique_ptr<IOpenGLRenderer>> renderers) {
  /*
   * If renderers are already initialized, release old GL resources first.
   *
   * In many Qt Quick paths, updatePaintNode() may not have a current GL
   * context. So this only releases if a context is available.
   */
  if (m_initialized) {
    auto *context = QOpenGLContext::currentContext();
    auto *f = context ? context->extraFunctions() : nullptr;

    if (f) {
      releaseRenderers(f);
    }

    m_initialized = false;
  }

  m_renderers = std::move(renderers);
}

void ChartRenderNode::setRenderPackage(ChartRenderPackage package) {
  /*
   * Apply payloads to node-owned renderers.
   *
   * ChartView builds the package.
   * ChartRenderNode owns the renderer objects.
   */
  for (auto &payload : package.seriesPayloads) {
    if (payload.seriesIndex < 0 ||
        payload.seriesIndex >= static_cast<int>(m_renderers.size())) {
      continue;
    }

    if (!payload.data || !m_renderers[payload.seriesIndex]) {
      continue;
    }

    m_renderers[payload.seriesIndex]->setData(std::move(payload.data));
  }

  m_plotContext = std::move(package.plotContext);
}

void ChartRenderNode::initDebugLogger(QOpenGLContext *context) {
  if (!qEnvironmentVariableIsSet("OPENGL_DEBUG")) {
    return;
  }

  if (m_debugLogger) {
    return;
  }

  m_debugLogger = std::make_unique<QOpenGLDebugLogger>();

  if (!m_debugLogger->initialize()) {
    CP_WARN("ChartRenderNode::initDebugLogger: OpenGL debug logger failed to "
            "initialize");
    m_debugLogger.reset();
    return;
  }

  QObject::connect(m_debugLogger.get(), &QOpenGLDebugLogger::messageLogged,
                   [](const QOpenGLDebugMessage &message) {
                     CP_WARN("OpenGL Debug: {}",
                             message.message().toStdString());
                   });

  m_debugLogger->startLogging(QOpenGLDebugLogger::SynchronousLogging);

  CP_DEBUG("ChartRenderNode::initDebugLogger: OpenGL debug logger initialized");
}

void ChartRenderNode::releaseDebugLogger() {
  if (m_debugLogger) {
    m_debugLogger->stopLogging();
    m_debugLogger.reset();
  }
}

void ChartRenderNode::initializeRenderers(QOpenGLExtraFunctions *f) {
  for (const auto &renderer : m_renderers) {
    if (renderer) {
      renderer->initialize(f);
    }
  }
}

void ChartRenderNode::renderRenderers(const ChartRenderContext &context) {
  for (const auto &renderer : m_renderers) {
    if (renderer) {
      renderer->render(context);
    }
  }
}

void ChartRenderNode::releaseRenderers(QOpenGLExtraFunctions *f) {
  for (const auto &renderer : m_renderers) {
    if (renderer) {
      renderer->release(f);
    }
  }
}

} // namespace ChartPlotter

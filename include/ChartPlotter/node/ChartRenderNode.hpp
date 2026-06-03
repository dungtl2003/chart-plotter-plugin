#pragma once

#include "ChartPlotter/ChartRenderContext.hpp"
#include "ChartPlotter/renderer/IOpenGLRenderer.hpp"

#include <QOpenGLDebugLogger>
#include <QSGRenderNode>

#include <memory>
#include <vector>

namespace ChartPlotter {

class ChartRenderNode : public QSGRenderNode {
public:
  ChartRenderNode();
  ~ChartRenderNode() override;

  StateFlags changedStates() const override;
  void prepare() override;
  void render(const RenderState *state) override;
  void releaseResources() override;
  RenderingFlags flags() const override;

  void setRenderers(std::vector<std::unique_ptr<IOpenGLRenderer>> renderers);
  void setRenderPackage(ChartRenderPackage package);

private:
  PlotContext m_plotContext;

  std::vector<std::unique_ptr<IOpenGLRenderer>> m_renderers;
  std::unique_ptr<QOpenGLDebugLogger> m_debugLogger;

  bool m_initialized = false;

private:
  void initDebugLogger(QOpenGLContext *context);
  void releaseDebugLogger();

  void initializeRenderers(QOpenGLExtraFunctions *f);
  void renderRenderers(const ChartRenderContext &context);
  void releaseRenderers(QOpenGLExtraFunctions *f);
};

} // namespace ChartPlotter

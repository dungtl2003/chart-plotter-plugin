#pragma once

#include "ChartPlotter/data/BarRenderData.hpp"
#include "ChartPlotter/renderer/IOpenGLRenderer.hpp"

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QVector2D>

#include <memory>

namespace ChartPlotter {

struct BarVertex {
  QVector2D position; // expanded quad corner (screen space)
};

class OpenGLBarRenderer : public IOpenGLRenderer {
public:
  OpenGLBarRenderer() = default;

  void initialize(QOpenGLExtraFunctions *f) override;
  void release(QOpenGLExtraFunctions *f) override;
  void render(const ChartRenderContext &context) override;
  void setData(std::unique_ptr<RenderData> data) override;

private:
  std::unique_ptr<QOpenGLShaderProgram> m_program;
  GLuint m_vao = 0;
  GLuint m_vbo = 0;
  std::unique_ptr<BarRenderData> m_data;

  QVector<BarVertex> m_vertices;

  void initializeProgram();
  void initializeGeometry(QOpenGLExtraFunctions *f);
  void buildVertices(const ChartRenderContext &context);
};

} // namespace ChartPlotter

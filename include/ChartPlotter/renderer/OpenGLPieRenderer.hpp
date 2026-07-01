#pragma once

#include "ChartPlotter/data/PieRenderData.hpp"
#include "ChartPlotter/renderer/IOpenGLRenderer.hpp"

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QVector2D>
#include <QVector4D>

#include <memory>

namespace ChartPlotter {

struct PieVertex {
  QVector2D position; // screen space
  QVector4D color;    // straight (non-premultiplied) RGBA
};

class OpenGLPieRenderer : public IOpenGLRenderer {
public:
  OpenGLPieRenderer() = default;

  void initialize(QOpenGLExtraFunctions *f) override;
  void release(QOpenGLExtraFunctions *f) override;
  void render(const ChartRenderContext &context) override;
  void setData(std::unique_ptr<RenderData> data) override;

private:
  std::unique_ptr<QOpenGLShaderProgram> m_program;
  GLuint m_vao = 0;
  GLuint m_vbo = 0;
  std::unique_ptr<PieRenderData> m_data;

  QVector<PieVertex> m_vertices;

  void initializeProgram();
  void initializeGeometry(QOpenGLExtraFunctions *f);
  void buildVertices(const ChartRenderContext &context);
};

} // namespace ChartPlotter

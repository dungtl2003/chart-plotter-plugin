#pragma once

#include "ChartPlotter/data/LineRenderData.hpp"
#include "ChartPlotter/renderer/IOpenGLRenderer.hpp"

#include <QColor>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QVector2D>

namespace ChartPlotter {

struct StrokeVertex {
  QVector2D position;
};

struct MarkerVertex {
  QVector2D position;
};

class OpenGLLineRenderer : public IOpenGLRenderer {
public:
  OpenGLLineRenderer() = default;

  void initialize(QOpenGLExtraFunctions *f) override;
  void release(QOpenGLExtraFunctions *f) override;
  void render(const ChartRenderContext &context) override;
  void setData(std::unique_ptr<RenderData> data) override;

private:
  std::unique_ptr<QOpenGLShaderProgram> m_strokeProgram;
  std::unique_ptr<QOpenGLShaderProgram> m_markerProgram;
  GLuint m_strokeVao = 0;
  GLuint m_strokeVbo = 0;
  GLuint m_markerVao = 0;
  GLuint m_markerVbo = 0;
  std::unique_ptr<LineRenderData> m_data;

  QVector<StrokeVertex> m_strokeVertices;
  QVector<MarkerVertex> m_markerVertices;

  void buildStrokeVertices(const QVector<QVector2D> &points);
  void buildSegmentVertices(const QVector<QVector2D> &points,
                            QVector<StrokeVertex> &outVertices);
  void buildMiterJoinVertices(const QVector<QVector2D> &points,
                              QVector<StrokeVertex> &outVertices);
  void buildMarkerVertices(const QVector<QVector2D> &points);

  void uploadStrokeVertices(QOpenGLExtraFunctions *f);
  void uploadMarkerVertices(QOpenGLExtraFunctions *f);

  void drawStrokesAsTriangles(QOpenGLExtraFunctions *f);
  void drawMarkersAsTriangles(QOpenGLExtraFunctions *f);

  void initializeStrokeGeometry(QOpenGLExtraFunctions *f);
  void initializeMarkerGeometry(QOpenGLExtraFunctions *f);

  void bindStrokeProgram(const QMatrix4x4 &mvp);
  void bindMarkerProgram(const QMatrix4x4 &mvp);

  void initializePrograms();
};

} // namespace ChartPlotter

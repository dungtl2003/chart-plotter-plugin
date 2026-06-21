#pragma once

#include "ChartPlotter/data/LineRenderData.hpp"
#include "ChartPlotter/renderer/IOpenGLRenderer.hpp"

#include <QColor>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QVector2D>

namespace ChartPlotter {

struct DashRun {
  QVector<QVector2D> points;
};

struct LineStrokeVertex {
  QVector2D position; // expanded quad corner (screen space)
  QVector2D p0;       // capsule segment start (screen space)
  QVector2D p1;       // capsule segment end   (screen space)
};

struct MarkerVertex {
  QVector2D position;
  QVector2D center;
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

  QVector<LineStrokeVertex> m_strokeVertices;
  QVector<MarkerVertex> m_markerVertices;

  void buildStrokeVertices(const QVector<QVector2D> &points);
  void buildSolidStrokeVertices(const QVector<QVector2D> &points,
                                QVector<LineStrokeVertex> &outVertices);
  void buildDashedStrokeVertices(const QVector<QVector2D> &points,
                                 QVector<LineStrokeVertex> &outVertices);
  void buildDottedStrokeVertices(const QVector<QVector2D> &points,
                                 QVector<LineStrokeVertex> &outVertices);
  void buildMarkerVertices(const QVector<QVector2D> &points);

  void drawStrokes(QOpenGLExtraFunctions *f, const ChartRenderContext &context);
  void drawMarkers(QOpenGLExtraFunctions *f, const ChartRenderContext &context);

  void initializeStrokeGeometry(QOpenGLExtraFunctions *f);
  void initializeMarkerGeometry(QOpenGLExtraFunctions *f);

  void bindStrokeProgram(const QMatrix4x4 &mvp);
  void bindMarkerProgram(const QMatrix4x4 &mvp);

  void initializePrograms();
};

} // namespace ChartPlotter

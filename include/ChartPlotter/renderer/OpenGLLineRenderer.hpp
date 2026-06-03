#pragma once

#include "ChartPlotter/data/LineRenderData.hpp"
#include "ChartPlotter/renderer/IOpenGLRenderer.hpp"

#include <QColor>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QVector2D>

namespace ChartPlotter {

struct StrokeSdfVertex {
  QVector2D position;
  float sdf;
};

struct MarkerSdfVertex {
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
  GLuint m_strokeCoreVao = 0;
  GLuint m_strokeCoreVbo = 0;
  GLuint m_strokeFringeVao = 0;
  GLuint m_strokeFringeVbo = 0;
  GLuint m_markerVao = 0;
  GLuint m_markerVbo = 0;
  std::unique_ptr<LineRenderData> m_data;

  QVector<StrokeSdfVertex> m_strokeCoreVertices;
  QVector<StrokeSdfVertex> m_strokeFringeVertices;
  QVector<MarkerSdfVertex> m_markerVertices;

  void buildStrokeVertices(const QVector<QVector2D> &points);
  void buildStrokeCoreVertices(const QVector<QVector2D> &points);
  void buildSegmentCoreVertices(const QVector<QVector2D> &points);
  void buildMiterJoinCoreVertices(const QVector<QVector2D> &points);
  void buildStrokeFringeVertices(const QVector<QVector2D> &points);
  void buildSegmentFringeVertices(const QVector<QVector2D> &points);
  void buildMiterJoinFringeVertices(const QVector<QVector2D> &points);
  void buildMarkerVertices(const QVector<QVector2D> &points);

  void uploadStrokeVertices(QOpenGLExtraFunctions *f);
  void uploadStrokeCoreVertices(QOpenGLExtraFunctions *f);
  void uploadStrokeFringeVertices(QOpenGLExtraFunctions *f);
  void uploadMarkerVertices(QOpenGLExtraFunctions *f);

  void drawStrokeCoresAsTriangles(QOpenGLExtraFunctions *f);
  void drawStrokeFringesAsTriangles(QOpenGLExtraFunctions *f);
  void drawMarkersAsTriangles(QOpenGLExtraFunctions *f);

  void initializeStrokeGeometry(QOpenGLExtraFunctions *f);
  void initializeStrokeCoreGeometry(QOpenGLExtraFunctions *f);
  void initializeStrokeFringeGeometry(QOpenGLExtraFunctions *f);
  void initializeMarkerGeometry(QOpenGLExtraFunctions *f);

  void bindStrokeProgram(const QMatrix4x4 &mvp);
  void bindMarkerProgram(const QMatrix4x4 &mvp);

  void initializePrograms();
};

} // namespace ChartPlotter

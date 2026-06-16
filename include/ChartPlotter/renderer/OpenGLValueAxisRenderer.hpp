#pragma once

#include "ChartPlotter/data/ValueAxisRenderData.hpp"
#include "ChartPlotter/renderer/IOpenGLRenderer.hpp"

#include <QFont>
#include <QHash>
#include <QOpenGLShaderProgram>

namespace ChartPlotter {

struct AxisStrokeVertex {
  QVector2D position;
};
struct TextVertex {
  QVector2D position;
  QVector2D texCoord;
};

class OpenGLValueAxisRenderer : public IOpenGLRenderer {
public:
  OpenGLValueAxisRenderer() = default;

  void initialize(QOpenGLExtraFunctions *f) override;
  void release(QOpenGLExtraFunctions *f) override;
  void render(const ChartRenderContext &context) override;
  void setData(std::unique_ptr<RenderData> data) override;

private:
  struct Tick {
    QVector2D position;
    QString label;
  };
  struct LabelTexture {
    GLuint id = 0;
    float width = 0.0f;
    float height = 0.0f;
  };
  struct LabelBatch {
    GLuint texture = 0;
    int firstVertex = 0;
    int vertexCount = 0;
  };

  std::unique_ptr<QOpenGLShaderProgram> m_strokeProgram;
  GLuint m_strokeVao = 0;
  GLuint m_strokeVbo = 0;
  std::unique_ptr<ValueAxisRenderData> m_data;

  std::unique_ptr<QOpenGLShaderProgram> m_textProgram;
  GLuint m_textVao = 0;
  GLuint m_textVbo = 0;
  QVector<TextVertex> m_labelVertices;
  QVector<LabelBatch> m_labelBatches;
  QHash<QString, LabelTexture> m_labelTextureCache;
  QFont m_labelFont;

  QVector<AxisStrokeVertex> m_axisAndTickVertices;
  QVector<AxisStrokeVertex> m_gridVertices;

  LabelTexture acquireLabelTexture(const QString &text,
                                   QOpenGLExtraFunctions *f, float dpr);
  LabelTexture rasterizeLabel(const QString &text, QOpenGLExtraFunctions *f,
                              float dpr);

  void buildAxisAndTickVertices(const ChartRenderContext &context,
                                const QVector<Tick> &ticks);
  void buildAxisVertices(QVector<AxisStrokeVertex> &out,
                         const QVector2D &firstPoint,
                         const QVector2D &lastPoint);
  void buildTickVertices(QVector<AxisStrokeVertex> &out,
                         const QVector<Tick> &ticks);
  void buildGridVertices(const ChartRenderContext &context,
                         const QVector<Tick> &ticks);
  void buildLabelVertices(const ChartRenderContext &context,
                          const QVector<Tick> &ticks, QOpenGLExtraFunctions *f,
                          float dpr);

  void uploadStrokeVertices(QOpenGLExtraFunctions *f,
                            const QVector<AxisStrokeVertex> &vertices);

  void drawStrokesAsTriangles(QOpenGLExtraFunctions *f,
                              const QVector<AxisStrokeVertex> &vertices);
  void drawLabels(QOpenGLExtraFunctions *f, const QMatrix4x4 &mvp,
                  const QColor &color);

  void initializeStrokeGeometry(QOpenGLExtraFunctions *f);
  void initializeTextGeometry(QOpenGLExtraFunctions *f);

  void bindStrokeProgram(const QMatrix4x4 &mvp, const QColor &color);

  void initializePrograms();
};

} // namespace ChartPlotter

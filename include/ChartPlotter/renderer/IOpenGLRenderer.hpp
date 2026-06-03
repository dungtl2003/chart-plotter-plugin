#pragma once

#include "ChartPlotter/ChartRenderContext.hpp"
#include "ChartPlotter/data/RenderData.hpp"

#include <QOpenGLContext>

namespace ChartPlotter {

class IOpenGLRenderer {
public:
  IOpenGLRenderer() = default;

  explicit IOpenGLRenderer(const IOpenGLRenderer &) = delete;
  explicit IOpenGLRenderer(IOpenGLRenderer &&) = default;
  IOpenGLRenderer &operator=(const IOpenGLRenderer &) = delete;
  IOpenGLRenderer &operator=(IOpenGLRenderer &&) = default;
  virtual ~IOpenGLRenderer() = default;

  virtual void initialize(QOpenGLExtraFunctions *f) = 0;
  virtual void release(QOpenGLExtraFunctions *f) = 0;
  virtual void render(const ChartRenderContext &context) = 0;

  virtual void setData(std::unique_ptr<RenderData> data) = 0;
};

} // namespace ChartPlotter

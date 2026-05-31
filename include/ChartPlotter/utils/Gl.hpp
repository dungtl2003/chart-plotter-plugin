#pragma once

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>

namespace ChartPlotter {

namespace Gl {

void checkError(QOpenGLExtraFunctions *f, const char *where);
QString readShaderSource(const QString &path);
std::unique_ptr<QOpenGLShaderProgram>
createProgram(const QString &vertexShader, const QString &fragmentShader,
              const char *debugName);

} // namespace Gl

} // namespace ChartPlotter

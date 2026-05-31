#include "ChartPlotter/utils/Gl.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

#include <QFile>

namespace ChartPlotter {

void Gl::checkError(QOpenGLExtraFunctions *f, const char *where) {
  if (!qEnvironmentVariableIsSet("OPENGL_DEBUG")) {
    return;
  }

  GLenum error;

  while ((error = f->glGetError()) != GL_NO_ERROR) {
    const char *name = "UNKNOWN";

    switch (error) {
    case GL_INVALID_ENUM:
      name = "GL_INVALID_ENUM";
      break;
    case GL_INVALID_VALUE:
      name = "GL_INVALID_VALUE";
      break;
    case GL_INVALID_OPERATION:
      name = "GL_INVALID_OPERATION";
      break;
    case GL_OUT_OF_MEMORY:
      name = "GL_OUT_OF_MEMORY";
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      name = "GL_INVALID_FRAMEBUFFER_OPERATION";
      break;
    }

    CP_WARN("Gl::checkOpenGLError: OpenGL error at {}: {} ({})", where, name,
            static_cast<unsigned int>(error));
  }
}

QString Gl::readShaderSource(const QString &path) {
  QFile file(path);

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    CP_WARN("Gl::readTextResource: Failed to open resource at {}: {}",
            path.toStdString(), file.errorString().toStdString());
    return {};
  }

  return QString::fromUtf8(file.readAll());
}

std::unique_ptr<QOpenGLShaderProgram>
Gl::createProgram(const QString &vertexShader, const QString &fragmentShader,
                  const char *debugName) {
  auto program = std::make_unique<QOpenGLShaderProgram>();

  if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader)) {
    CP_WARN("{}: vertex shader error: {}", debugName,
            program->log().toStdString());
    return nullptr;
  }

  if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                        fragmentShader)) {
    CP_WARN("{}: fragment shader error: {}", debugName,
            program->log().toStdString());
    return nullptr;
  }

  if (!program->link()) {
    CP_WARN("{}: shader link error: {}", debugName,
            program->log().toStdString());
    return nullptr;
  }

  return program;
}

} // namespace ChartPlotter

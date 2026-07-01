#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[]) {
  QSurfaceFormat format;
  format.setRenderableType(QSurfaceFormat::OpenGL);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setVersion(3, 3);
  format.setSamples(8);

  // Important for OpenGL debug messages
  format.setOption(QSurfaceFormat::DebugContext);

  QSurfaceFormat::setDefaultFormat(format);

  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;

  // Load the plugin we fetched via CMake
  engine.addImportPath(PLUGIN_IMPORT_PATH);
  qDebug() << "Loaded Plugin from:" << PLUGIN_IMPORT_PATH;

  const QUrl url(u"qrc:/qt/qml/VeryLargeDatasetChart/qml/Main.qml"_s);
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
  engine.load(url);

  return app.exec();
}

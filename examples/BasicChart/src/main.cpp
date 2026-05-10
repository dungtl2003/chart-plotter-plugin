#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;

  // Load the plugin we fetched via CMake
  engine.addImportPath(PLUGIN_IMPORT_PATH);
  qDebug() << "Loaded Plugin from:" << PLUGIN_IMPORT_PATH;

  const QUrl url(u"qrc:/qt/qml/Main/qml/Main.qml"_s);
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
  engine.load(url);

  return app.exec();
}

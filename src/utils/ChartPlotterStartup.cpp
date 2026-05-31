#include "ChartPlotter/utils/LoggerManager.hpp"

#include <QCoreApplication>
#include <QObject>
#include <qlogging.h>

namespace ChartPlotter {

static void initChartPlotter() {

  LoggerManager::init();

  QObject::connect(qApp, &QCoreApplication::aboutToQuit,
                   []() { LoggerManager::shutdown(); });
}

Q_COREAPP_STARTUP_FUNCTION(initChartPlotter)

}; // namespace ChartPlotter

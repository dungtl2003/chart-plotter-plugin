#pragma once

#include <QQmlEngineExtensionPlugin>
#include <QtQml/qqmlextensionplugin.h>

class ChartPlotterPlugin : public QQmlEngineExtensionPlugin {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)

public:
  ChartPlotterPlugin();
  ~ChartPlotterPlugin() override;
};

#include "ChartModel.hpp"
#include <spdlog/spdlog.h>

ChartModel::ChartModel(QObject *parent) : QObject(parent) {
  spdlog::info("ChartModel successfully initialized!");
  spdlog::warn("Warning: No initial data loaded.");
}

QString ChartModel::provider() const { return "OpenGL C++ Engine v1.0"; }

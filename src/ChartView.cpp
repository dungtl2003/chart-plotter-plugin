#include "ChartPlotter/ChartView.hpp"

#include "ChartPlotter/axis/ValueAxis.hpp"
#include "ChartPlotter/data/RenderData.hpp"
#include "ChartPlotter/data/ValueAxisRenderData.hpp"
#include "ChartPlotter/node/ChartRenderNode.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "factory/SeriesComponentFactoryProvider.hpp"

namespace ChartPlotter {

ChartView::ChartView(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);

  m_name = QString::fromStdString(appendUniqueId("ChartView_EarlyInit_"));
  m_logger = LoggerManager::createInstanceLogger(m_name.toStdString());
}

ChartView::~ChartView() {
  shutdownDataManagers();

  if (m_logger) {
    dropLogger();
  }
}

void ChartView::componentComplete() {
  QQuickItem::componentComplete();

  m_plan = ChartLayoutPlanner::buildPlan(m_series);
  if (!m_plan.valid) {
    m_logger->warn("ChartView::componentComplete: {}",
                   m_plan.errorMessage.toStdString());
    return;
  }

  for (auto source : m_sources) {
    createDataManager(source);
  }
}

void ChartView::onDataError(const QString &message) {
  m_logger->warn(message.toStdString());
}

void ChartView::onSnapshotReady(int sourceId, const DataSnapshot &snapshot) {
  // m_logger->debug(snapshot.toString().toStdString());

  m_snapshots[sourceId] = snapshot;

  if (!m_plan.valid) {
    return;
  }

  if (!rebuildRenderPackage()) {
    return;
  }

  update();
}

QSGNode *ChartView::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
  auto *node = static_cast<ChartRenderNode *>(oldNode);

  if (!node) {
    node = new ChartRenderNode();
    node->setRenderers(createRenderersFromPlan());
  }

  if (m_pendingRenderPackage.has_value()) {
    node->setRenderPackage(std::move(*m_pendingRenderPackage));
    m_pendingRenderPackage.reset();
  }

  m_plotContext.itemRect = boundingRect();
  m_plotContext.plotArea =
      m_plotContext.itemRect.adjusted(100, 100, -100, -100);
  node->setPlotContext(m_plotContext);

  return node;
}

QQmlListProperty<QObject> ChartView::content() {
  return QQmlListProperty<QObject>(
      this, nullptr, &ChartView::appendContent, &ChartView::contentCount,
      &ChartView::contentAt, &ChartView::clearContent);
}

void ChartView::appendContent(QQmlListProperty<QObject> *property,
                              QObject *object) {
  auto *chartView = qobject_cast<ChartView *>(property->object);

  if (!chartView || !object) {
    return;
  }

  chartView->m_content.push_back(object);

  if (auto *series = qobject_cast<AbstractSeries *>(object)) {
    chartView->m_series.push_back(series);
    auto factory = SeriesComponentFactoryProvider::getFactory(series->type());
    if (!factory) {
      chartView->m_logger->warn(
          "ChartView::appendContent: unsupport chart type");
      chartView->m_strategies.push_back(nullptr);
      return;
    }

    auto strategy = factory->getStrategy();
    if (!strategy) {
      chartView->m_strategies.push_back(nullptr);
      return;
    }

    chartView->m_strategies.push_back(std::move(strategy));
  }

  if (auto *source = qobject_cast<DataSource *>(object)) {
    chartView->m_sources.push_back(source);
  }
}

QPointer<DataManager>
ChartView::createDataManager(const QPointer<DataSource> source) {
  const int id = m_nextSourceId++;

  m_sourceIds.insert(source.data(), id);

  QPointer<QThread> thread = new QThread(this);

  QPointer<DataManager> manager = new DataManager();
  manager->setDataReadConfig(std::move(source->exportConfig()));
  manager->moveToThread(thread);

  connect(thread, &QThread::started, manager, &DataManager::start);
  connect(thread, &QThread::finished, manager, &QObject::deleteLater);
  connect(thread, &QThread::finished, this, [this, manager, thread]() {
    this->shutdownDataManager(manager, thread);
  });
  connect(manager, &DataManager::errorOccurred, this, &ChartView::onDataError,
          Qt::QueuedConnection);
  connect(
      manager, &DataManager::snapshotReady, this,
      [this, id](const DataSnapshot &snapshot) {
        onSnapshotReady(id, snapshot);
      },
      Qt::QueuedConnection);
  connect(manager, &DataManager::finished, thread, &QThread::quit,
          Qt::QueuedConnection);

  m_dataManagers.push_back({id, thread, manager});

  thread->start();

  return manager;
}

void ChartView::stopDataManager(int id) {
  for (DataManagerRuntime &runtime : m_dataManagers) {
    if (runtime.id != id) {
      continue;
    }

    if (runtime.manager) {
      QMetaObject::invokeMethod(runtime.manager, "stop",
                                Qt::BlockingQueuedConnection);
    }

    return;
  }
}

void ChartView::shutdownDataManagers() {
  for (DataManagerRuntime &runtime : m_dataManagers) {
    if (!runtime.manager) {
      continue;
    }

    QPointer<DataManager> manager = runtime.manager;
    QMetaObject::invokeMethod(manager, "stop", Qt::BlockingQueuedConnection);

    if (runtime.thread) {
      runtime.thread->quit();
      runtime.thread->wait();
    }

    runtime.manager = nullptr;
    runtime.thread = nullptr;
  }

  m_dataManagers.clear();
}

void ChartView::shutdownDataManager(QPointer<DataManager> manager,
                                    QPointer<QThread> thread) {
  for (auto &runtime : m_dataManagers) {
    if (runtime.manager == manager) {
      runtime.manager = nullptr;
      runtime.thread = nullptr;
      break;
    }
  }

  thread->deleteLater();
}

// TODO: handle xy series for now
bool ChartView::rebuildRenderPackage() {
  assert(m_plan.valid);

  m_resolvedSeries = m_seriesDataResolver.resolve(
      m_plan.xySeriesIndexes, m_plan.pieSeriesIndexes, m_series, m_sourceIds,
      m_snapshots);

  if (!m_resolvedSeries.valid) {
    m_logger->warn("ChartView::rebuildRenderPackage: {}",
                   m_resolvedSeries.errorMessage.toStdString());
    return false;
  }

  if (m_plan.layoutType == SeriesLayoutType::XY) {
    return rebuildXYSeriesRenderPackage(m_resolvedSeries);
  }

  if (m_plan.layoutType == SeriesLayoutType::Pie) {
    return rebuildPieSeriesRenderPackage(m_resolvedSeries);
  }

  m_logger->warn("ChartView::rebuildRenderPackage: unsupported layout type");
  return false;
}

bool ChartView::rebuildXYSeriesRenderPackage(
    const SeriesResolveResult &resolvedResult) {
  ChartRenderPackage package;

  DataRange globalX;
  DataRange globalY;

  for (const ResolvedSeriesData &resolved : resolvedResult.xySeries) {
    if (!resolved.valid) {
      m_logger->warn("ChartView::rebuildXYSeriesRenderPackage: {}",
                     resolved.errorMessage.toStdString());
      continue;
    }

    const int seriesIndex = resolved.seriesIndex;

    if (seriesIndex < 0 || seriesIndex >= m_series.size()) {
      m_logger->warn(
          "ChartView::rebuildXYSeriesRenderPackage: invalid series index {}",
          seriesIndex);
      continue;
    }

    if (seriesIndex >= static_cast<int>(m_strategies.size()) ||
        !m_strategies[seriesIndex]) {
      m_logger->warn(
          "ChartView::rebuildXYSeriesRenderPackage: missing strategy for "
          "series index {}",
          seriesIndex);
      continue;
    }

    QPointer<AbstractSeries> series = m_series[seriesIndex];
    if (!series) {
      continue;
    }

    auto snapshotIt = m_snapshots.constFind(resolved.sourceId);

    if (snapshotIt == m_snapshots.constEnd()) {
      m_logger->warn(
          "ChartView::rebuildXYSeriesRenderPackage: missing snapshot for "
          "sourceId {}",
          resolved.sourceId);
      continue;
    }

    const DataSnapshot &snapshot = snapshotIt.value();

    std::unique_ptr<RenderData> data =
        m_strategies[seriesIndex]->build(*series, resolved, snapshot);

    if (!data) {
      m_logger->warn(
          "ChartView::rebuildXYSeriesRenderPackage: strategy returned null "
          "render data for series index {}",
          seriesIndex);
      continue;
    }

    auto *xyData = dynamic_cast<XYSeriesRenderData *>(data.get());

    if (!xyData) {
      m_logger->warn(
          "ChartView::rebuildXYSeriesRenderPackage: render data is not "
          "XYSeriesRenderData for series index {}",
          seriesIndex);
      continue;
    }

    if (!xyData->xRange.valid || !xyData->yRange.valid) {
      m_logger->warn(
          "ChartView::rebuildXYSeriesRenderPackage: invalid data range for "
          "series index {}",
          seriesIndex);
      continue;
    }

    globalX = unionRange(globalX, xyData->xRange);
    globalY = unionRange(globalY, xyData->yRange);

    SeriesRenderPayload payload;
    payload.seriesIndex = seriesIndex;
    payload.data = std::move(data);

    package.seriesPayloads.push_back(std::move(payload));
  }

  if (package.seriesPayloads.empty()) {
    m_logger->warn(
        "ChartView::rebuildXYSeriesRenderPackage: no valid XY series render "
        "payloads");
    return false;
  }

  if (!globalX.valid || !globalY.valid) {
    m_logger->warn(
        "ChartView::rebuildXYSeriesRenderPackage: global XY range is invalid");
    return false;
  }

  m_plotContext.xRange = globalX;
  m_plotContext.yRange = globalY;

  // TODO: handle category axis as well
  ValueAxisRange xAxisRange = ValueAxis::calculateRange(globalX);
  ValueAxisRange yAxisRange = ValueAxis::calculateRange(globalY);
  ValueAxisTicks xAxisTicks = ValueAxis::calculateTicks(xAxisRange);
  ValueAxisTicks yAxisTicks = ValueAxis::calculateTicks(yAxisRange);
  m_plotContext.xAxisRange = xAxisRange;
  m_plotContext.yAxisRange = yAxisRange;
  package.xAxisPayload = AxisPayload{
      .ticks = std::move(xAxisTicks),
      .range = std::move(xAxisRange),
      .data = std::make_unique<ValueAxisRenderData>(),
  };
  package.yAxisPayload = AxisPayload{
      .ticks = std::move(yAxisTicks),
      .range = std::move(yAxisRange),
      .data = std::make_unique<ValueAxisRenderData>(),
  };

  m_pendingRenderPackage = std::move(package);
  return true;
}

// TODO
bool ChartView::rebuildPieSeriesRenderPackage(
    const SeriesResolveResult &resolvedSeries) {
  return false;
}

std::vector<std::unique_ptr<IOpenGLRenderer>>
ChartView::createRenderersFromPlan() const {
  std::vector<std::unique_ptr<IOpenGLRenderer>> renderers;
  renderers.reserve(m_series.size());

  for (const auto &series : m_series) {
    if (!series) {
      renderers.push_back(nullptr);
      continue;
    }

    auto factory = SeriesComponentFactoryProvider::getFactory(series->type());

    if (!factory) {
      renderers.push_back(nullptr);
      continue;
    }

    renderers.push_back(factory->getRenderer());
  }

  return renderers;
}

qsizetype ChartView::contentCount(QQmlListProperty<QObject> *property) {
  auto *chartView = qobject_cast<ChartView *>(property->object);

  if (!chartView) {
    return 0;
  }

  return chartView->m_content.size();
}

DataRange ChartView::unionRange(const DataRange &a, const DataRange &b) const {
  if (!a.valid) {
    return b;
  }

  if (!b.valid) {
    return a;
  }

  DataRange result = a;
  result.min = std::min(a.min, b.min);
  result.max = std::max(a.max, b.max);
  result.valid = true;

  return result;
}

QObject *ChartView::contentAt(QQmlListProperty<QObject> *property,
                              qsizetype index) {
  auto *chartView = qobject_cast<ChartView *>(property->object);

  if (!chartView) {
    return nullptr;
  }

  if (index < 0 || index >= chartView->m_content.size()) {
    return nullptr;
  }

  return chartView->m_content.at(index);
}

void ChartView::clearContent(QQmlListProperty<QObject> *property) {
  auto *chartView = qobject_cast<ChartView *>(property->object);

  if (!chartView) {
    return;
  }

  chartView->m_content.clear();
  chartView->m_sources.clear();
  chartView->m_series.clear();
  chartView->m_dataManagers.clear();
}

QString ChartView::name() const { return m_name; }

void ChartView::setName(QString newName) {
  if (m_name == newName) {
    return;
  }

  m_name = newName;
  emit nameChanged();

  if (m_logger) {
    dropLogger();
  }

  m_logger = LoggerManager::createInstanceLogger(
      appendUniqueId("ChartView_" + m_name.toStdString() + "_"));
}

void ChartView::resetStrategies() {
  for (auto &strat : m_strategies) {
    strat.reset();
  }
}

void ChartView::dropLogger() {
  if (!m_logger) {
    return;
  }

  std::string old_logger_name = m_logger->name();
  m_logger.reset();
  spdlog::drop(old_logger_name);
}

std::string ChartView::appendUniqueId(std::string s) const {
  std::stringstream ss;
  ss << s << this;

  return ss.str();
}

} // namespace ChartPlotter

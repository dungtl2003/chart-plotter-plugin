#include "ChartPlotter/ChartView.hpp"

#include "ChartPlotter/axis/ValueAxis.hpp"
#include "ChartPlotter/data/RenderData.hpp"
#include "ChartPlotter/data/ValueAxisRenderData.hpp"
#include "ChartPlotter/node/ChartRenderNode.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "factory/SeriesComponentFactoryProvider.hpp"

namespace ChartPlotter {

float GeneralConfig::lineWidth() const { return m_lineWidth; }
void GeneralConfig::setLineWidth(float newLineWidth) {
  m_lineWidth = newLineWidth;
}

bool GeneralConfig::operator==(const GeneralConfig &other) const {
  return m_lineWidth == other.lineWidth();
}
bool GeneralConfig::operator!=(const GeneralConfig &other) const {
  return m_lineWidth != other.lineWidth();
}

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

  ChartLayoutPlanner planner;
  m_plan = planner.buildPlan(m_series);
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

  if (m_plan.coordinateSystem == CoordinateSystem::Cartesian) {
    return rebuildXYSeriesRenderPackage(m_resolvedSeries);
  }

  if (m_plan.coordinateSystem == CoordinateSystem::Pie) {
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

  CategoryAxis xCategoryAxis;
  const bool xIsCategory =
      resolvedResult.sharedXColumnType == ChartEnums::DataType::String;
  if (xIsCategory) {
    buildCategoryAxis(resolvedResult.xySeries, true, xCategoryAxis);
  }

  SeriesBuildContext buildContext;
  buildContext.xCategories = xIsCategory ? &xCategoryAxis : nullptr;

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

    std::unique_ptr<RenderData> data = m_strategies[seriesIndex]->build(
        *series, resolved, snapshot, buildContext);

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

  AxisRange xAxisRange;
  AxisRange yAxisRange;
  QVector<AxisTick> xTicks;
  QVector<AxisTick> yTicks;

  // ---- X axis ----
  if (xIsCategory) {
    const int n = xCategoryAxis.size();
    // half-step padding keeps end categories off the frame; use {0, n-1}
    // instead if you don't want the edge gaps.
    xAxisRange = AxisRange{.min = 0, .max = double(n - 1)};
    xTicks.reserve(n);
    for (int i = 0; i < n; ++i) {
      xTicks.push_back({static_cast<double>(i), xCategoryAxis.labelAt(i)});
    }
  } else {
    const AxisRange base = ValueAxis::calculateRange(globalX);
    const AxisTicks ticks = ValueAxis::calculateTicks(
        base, 6,
        resolvedResult.sharedXColumnType == ChartEnums::DataType::Date);
    xAxisRange = ticks.ticks.isEmpty()
                     ? base
                     : AxisRange{.min = ticks.ticks.at(0).value,
                                 .max = ticks.ticks.last().value};
    xTicks.reserve(ticks.ticks.size());
    for (qsizetype i = 0; i < ticks.ticks.size(); ++i) {
      xTicks.push_back(
          {xAxisRange.min + i * ticks.step, ticks.ticks.at(i).label});
    }
  }

  // ---- Y axis (value path) ----
  {
    const AxisRange base = ValueAxis::calculateRange(globalY);
    const AxisTicks ticks = ValueAxis::calculateTicks(base);
    yAxisRange = ticks.ticks.isEmpty()
                     ? base
                     : AxisRange{.min = ticks.ticks.at(0).value,
                                 .max = ticks.ticks.last().value};
    yTicks.reserve(ticks.ticks.size());
    for (qsizetype i = 0; i < ticks.ticks.size(); ++i) {
      yTicks.push_back(
          {yAxisRange.min + i * ticks.step, ticks.ticks.at(i).label});
    }
  }

  m_plotContext.xAxisRange = xAxisRange;
  m_plotContext.yAxisRange = yAxisRange;
  m_plotContext.axisPositions =
      ChartEnums::AxisPosition::Left | ChartEnums::AxisPosition::Bottom;

  auto xData = std::make_unique<AxisRenderData>();
  xData->pos = ChartEnums::AxisPosition::Bottom;
  xData->ticks.reserve(xTicks.size());
  for (const AxisTick &t : xTicks) {
    xData->ticks.push_back(AxisTickRenderData{
        .pos = QPointF(t.value, yAxisRange.min),
        .label = t.label,
    });
  }

  auto yData = std::make_unique<AxisRenderData>();
  yData->pos = ChartEnums::AxisPosition::Left;
  yData->ticks.reserve(yTicks.size());
  for (const AxisTick &t : yTicks) {
    yData->ticks.push_back(AxisTickRenderData{
        .pos = QPointF(xAxisRange.min, t.value),
        .label = t.label,
    });
  }
  // CP_DEBUG(yData->toString().toStdString());
  package.xAxisPayload = AxisPayload{
      .data = std::move(xData),
  };
  package.yAxisPayload = AxisPayload{
      .data = std::move(yData),
  };

  m_pendingRenderPackage = std::move(package);
  return true;
}

// TODO
bool ChartView::rebuildPieSeriesRenderPackage(
    const SeriesResolveResult &resolvedSeries) {
  return false;
}

void ChartView::buildCategoryAxis(const QVector<ResolvedSeriesData> &xySeries,
                                  bool useX, CategoryAxis &axis) const {
  for (const ResolvedSeriesData &resolved : xySeries) {
    if (!resolved.valid) {
      continue;
    }
    auto it = m_snapshots.constFind(resolved.sourceId);
    if (it == m_snapshots.constEnd()) {
      continue;
    }
    const DataSnapshot &snapshot = it.value();
    const int col = useX ? resolved.xColumnIndex : resolved.yColumnIndex;
    if (col < 0 || col >= snapshot.columnCount) {
      continue;
    }
    for (int row = 0; row < snapshot.rowCount; ++row) {
      const QVariant v = snapshot.valueAt(col, row);
      if (!v.isValid() || v.isNull()) {
        continue;
      }
      axis.intern(v.toString());
    }
  }
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

GeneralConfig ChartView::generalConfig() const { return m_generalConfig; }
void ChartView::setGeneralConfig(const GeneralConfig &newConfig) {
  if (m_generalConfig == newConfig) {
    return;
  }

  m_generalConfig = newConfig;
  emit generalConfigChanged();
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

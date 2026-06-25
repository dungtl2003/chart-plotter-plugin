#include "ChartPlotter/ChartView.hpp"

#include "ChartPlotter/axis/AxisBuilder.hpp"
#include "ChartPlotter/constants/ChartConstants.hpp"
#include "ChartPlotter/data/RenderData.hpp"
#include "ChartPlotter/downsample/HybridDownsampler.hpp"
#include "ChartPlotter/downsample/LargestTriangleThreeBuckets.hpp"
#include "ChartPlotter/factory/SeriesComponentFactoryProvider.hpp"
#include "ChartPlotter/node/ChartRenderNode.hpp"
#include "ChartPlotter/utils/DataRangeCalculator.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

#include <algorithm>

namespace ChartPlotter {

float GeneralConfig::lineWidth() const { return m_lineWidth; }
void GeneralConfig::setLineWidth(float newLineWidth) {
  m_lineWidth = newLineWidth;
}

float GeneralConfig::antialiasing() const { return m_antialiasing; }
void GeneralConfig::setAntialiasing(float a) {
  m_antialiasing =
      std::clamp(a, ChartConstants::LINE_AA_MIN, ChartConstants::LINE_AA_MAX);
}

int GeneralConfig::xPreferredTickCount() const { return m_xPreferredTickCount; }
void GeneralConfig::setXPreferredTickCount(int tickCount) {
  m_xPreferredTickCount = std::clamp(tickCount, ChartConstants::TICK_COUNT_MIN,
                                     ChartConstants::TICK_COUNT_MAX);
}

int GeneralConfig::yPreferredTickCount() const { return m_yPreferredTickCount; }
void GeneralConfig::setYPreferredTickCount(int tickCount) {
  m_yPreferredTickCount = std::clamp(tickCount, ChartConstants::TICK_COUNT_MIN,
                                     ChartConstants::TICK_COUNT_MAX);
}

bool GeneralConfig::operator==(const GeneralConfig &o) const {
  return m_lineWidth == o.m_lineWidth && m_antialiasing == o.m_antialiasing &&
         m_xPreferredTickCount == o.m_xPreferredTickCount &&
         m_yPreferredTickCount == o.m_yPreferredTickCount;
}
bool GeneralConfig::operator!=(const GeneralConfig &o) const {
  return m_lineWidth != o.lineWidth() || m_antialiasing != o.m_antialiasing ||
         m_xPreferredTickCount != o.m_xPreferredTickCount ||
         m_yPreferredTickCount != o.m_yPreferredTickCount;
}

ChartView::ChartView(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);
  setAcceptedMouseButtons(Qt::LeftButton);

  m_updateTimer = new QTimer(this);
  m_updateTimer->setSingleShot(true);
  connect(m_updateTimer, &QTimer::timeout, this,
          &ChartView::performScheduledUpdate);
  m_lastUpdateTimer.start();

  m_dataManagerPool = new DataManagerPool(this);
  m_legendModel = new LegendModel(this);
  m_viewportController = new ViewportController(this);
  m_name = QString::fromStdString(appendUniqueId("ChartView_EarlyInit_"));
  m_logger = LoggerManager::createInstanceLogger(m_name.toStdString());
  connect(m_dataManagerPool, &DataManagerPool::errorOccurred, this,
          &ChartView::onDataError, Qt::QueuedConnection);
  // connect(
  //     m_dataManagerPool, &DataManagerPool::snapshotReady, this,
  //     [this](int id, const DataSnapshot &snapshot) {
  //       onSnapshotReady(id, snapshot);
  //     },
  //     Qt::QueuedConnection);
  connect(
      m_dataManagerPool, &DataManagerPool::snapshotsReady, this,
      [this](const std::vector<std::pair<int, DataSnapshot>> &snapshots) {
        onSnapshotsReady(snapshots);
      },
      Qt::QueuedConnection);

  connect(m_legendModel, &LegendModel::visibilityChanged, this,
          [this](int, bool) {
            if (m_plan.valid && rebuildRenderPackage()) {
              scheduleUpdate();
            }
          });
}

ChartView::~ChartView() {
  if (m_logger) {
    dropLogger();
  }
}

void ChartView::componentComplete() {
  QQuickItem::componentComplete();

  ChartLayoutPlanner planner;
  ChartChromeRequest chrome;
  chrome.hasTitle = !m_title.isEmpty();
  chrome.legendPosition = m_legendPosition;
  m_plan = planner.buildPlan(m_series, chrome);
  if (!m_plan.valid) {
    m_logger->warn("ChartView::componentComplete: {}",
                   m_plan.errorMessage.toStdString());
    return;
  }

  rebuildLegendModel();

  for (auto source : m_sources) {
    if (m_dataManagerPool) {
      m_dataManagerPool->createDataManager(source);
    }
  }

  relayout();

  // Force the initial empty render package build
  if (m_plan.valid && rebuildRenderPackage()) {
    scheduleUpdate();
  }
}

void ChartView::geometryChange(const QRectF &newGeom, const QRectF &oldGeom) {
  QQuickItem::geometryChange(newGeom, oldGeom);
  relayout();
}

void ChartView::wheelEvent(QWheelEvent *event) {
  // if (!(event->modifiers() & Qt::ControlModifier)) {
  //   QQuickItem::wheelEvent(event);
  //   return;
  // }

  /**
   * When you scroll a standard mouse wheel by one physical notch, the operating
   * system and Qt do not return 1. Instead, they return a standard metric value
   * of 120 units.
   *
   * This 120 value is an industry-standard constant designed to allow
   * high-precision mice or smooth-scrolling trackpads to send smaller
   * fractional movements (e.g., sending 30 units four times instead of one
   * large chunk of 120).
   *
   * Most standard mouse wheels have 24 notches in a full 360 degree rotation
   * circle.
   *
   * 360 degree / 24 notches = 15 degree per notch
   *
   * Because the hardware sends a value of 120 for that exact same notch, we can
   * find the mathematical relationship between hardware units and real-world
   * angles:
   *
   * 120 hardware units / 15 degree = 8
   *
   * Therefore, dividing the raw value by 8 converts hardware units directly
   * into geometric degrees.
   */
  QPoint numDegrees = event->angleDelta() / 8;
  QPoint numSteps = numDegrees / 15;
  QPointF mousePos = event->position();

  if (numSteps.isNull() || mousePos.isNull() || numSteps.y() == 0) {
    return;
  }

  if (!m_viewportController || !m_plotContext.plotArea.isValid()) {
    return;
  }

  if (!m_plotContext.plotArea.contains(mousePos)) {
    return;
  }

  m_viewportController->zoom(m_plotContext.plotArea, mousePos, numSteps.y());
  if (m_plan.valid && rebuildRenderPackage()) {
    scheduleUpdate();
  }

  event->accept();
}

void ChartView::mousePressEvent(QMouseEvent *event) {
  if (event->button() != Qt::LeftButton) {
    QQuickItem::mousePressEvent(event);
    return;
  }

  m_isPanning = true;
  m_panLastMousePos = event->position();
  m_lockedYRange = m_plotContext.yRange;
  grabMouse(); // Capture all mouse events
  event->accept();
}

void ChartView::mouseMoveEvent(QMouseEvent *event) {
  if (!m_isPanning || !(event->buttons() & Qt::LeftButton)) {
    QQuickItem::mouseMoveEvent(event);
    return;
  }

  QPointF mousePos = event->position();
  if (mousePos.isNull()) {
    QQuickItem::mouseMoveEvent(event);
    return;
  }

  if (!m_viewportController || !m_plotContext.plotArea.isValid()) {
    return;
  }

  if (!m_plotContext.plotArea.contains(mousePos)) {
    return;
  }

  QPointF delta = mousePos - m_panLastMousePos;
  const double deltaX = delta.x();
  if (std::abs(deltaX) == 0) {
    return;
  }

  m_viewportController->pan(m_plotContext.plotArea, deltaX);
  m_panLastMousePos = mousePos;
  if (m_plan.valid && rebuildRenderPackage()) {
    scheduleUpdate();
  }

  event->accept();
}

void ChartView::mouseReleaseEvent(QMouseEvent *event) {
  if (!m_isPanning) {
    QQuickItem::mouseReleaseEvent(event);
    return;
  }

  m_isPanning = false;

  if (m_plan.valid && rebuildRenderPackage()) {
    scheduleUpdate();
  }

  ungrabMouse(); // Release the mouse capture
  event->accept();
}

void ChartView::onDataError(const QString &message) {
  m_logger->warn(message.toStdString());
}

// void ChartView::onSnapshotReady(int sourceId, const DataSnapshot &snapshot) {
//   // m_logger->debug(snapshot.toString().toStdString());
//
//   CP_DEBUG("Received onSnapshotReady from DataManagerPool");
//   m_snapshots[sourceId] = snapshot;
//
//   if (!m_plan.valid) {
//     return;
//   }
//
//   if (!rebuildRenderPackage()) {
//     return;
//   }
//
//   scheduleUpdate();
// }

void ChartView::onSnapshotsReady(
    const std::vector<std::pair<int, DataSnapshot>> &snapshots) {
  bool hasNewChanges = false;
  // m_logger->debug(snapshot.toString().toStdString());
  for (const auto &p : snapshots) {
    if (!m_snapshots.contains(p.first) ||
        m_snapshots[p.first].epochId != p.second.epochId ||
        m_snapshots[p.first].version < p.second.version) {
      hasNewChanges = true;
      m_snapshots[p.first] = p.second;
    }
  }

  if (!hasNewChanges) {
    return;
  }

  if (!m_plan.valid) {
    return;
  }

  if (!rebuildRenderPackage()) {
    return;
  }

  scheduleUpdate();
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

  const QRectF outer =
      m_plotOuterRect.isValid() ? m_plotOuterRect : boundingRect();
  const auto m = m_plan.plotMargins;
  m_plotContext.itemRect = outer;
  m_plotContext.plotArea = outer.adjusted(m.left, m.top, -m.right, -m.bottom);
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

// TODO: handle xy series for now
bool ChartView::rebuildRenderPackage() {
  assert(m_plan.valid && m_dataManagerPool);

  // CP_DEBUG("resolving...");
  m_resolvedSeries = m_seriesDataResolver.resolve(
      m_plan.xySeriesIndexes, m_plan.pieSeriesIndexes, m_series,
      m_dataManagerPool->sourceIds(), m_snapshots, &m_globalPointCache);

  if (!m_resolvedSeries.valid) {
    m_logger->warn("ChartView::rebuildRenderPackage: {}",
                   m_resolvedSeries.errorMessage.toStdString());
    return false;
  }

  if (m_viewportController) {
    m_viewportController->setTargetTickCount(
        m_generalConfig.xPreferredTickCount());

    if (m_resolvedSeries.absoluteXRange.valid) {
      m_viewportController->setRange(m_resolvedSeries.absoluteXRange);
    }
  }

  if (m_plan.coordinateSystem == CoordinateSystem::Cartesian) {
    return rebuildXYSeriesRenderPackage(m_resolvedSeries);
  }

  if (m_plan.coordinateSystem == CoordinateSystem::Pie) {
    return rebuildPieSeriesRenderPackage(m_resolvedSeries);
  }

  m_logger->warn("ChartView::rebuildRenderPackage: unsupported layout type");
  // CP_DEBUG("FINISH rebuilding packages");
  return false;
}

bool ChartView::rebuildXYSeriesRenderPackage(
    const SeriesResolveResult &resolvedResult) {
  // CP_DEBUG("Start rebuilding XY Series Render package...");
  ChartRenderPackage package;

  /**
   * We will render all points limit by visible X range. Visible Y range will
   * depend on points that satisfied X range.
   */
  DataRange globalX = resolvedResult.absoluteXRange;
  // we will recalculate y range by points accept from x range
  DataRange globalY;

  const bool xIsCategory =
      resolvedResult.sharedXColumnType == ChartEnums::DataType::String;

  SeriesBuildContext buildContext;
  buildContext.xCategories =
      xIsCategory ? &resolvedResult.sharedXCategories : nullptr;
  buildContext.globalLineWidth = m_generalConfig.lineWidth();
  buildContext.globalAntialiasing = m_generalConfig.antialiasing();
  // buildContext.dataDownsampler =
  //     std::make_unique<LargestTriangleThreeBuckets>();
  buildContext.dataDownsampler = std::make_unique<HybridDownsampler>();
  buildContext.globalPointCache = &m_globalPointCache;

  if (m_viewportController && m_viewportController->getVisibleRange().valid) {
    buildContext.viewportXRange = m_viewportController->getVisibleRange();
    globalX = buildContext.viewportXRange;
  }

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

    if (m_legendModel && !m_legendModel->isVisible(seriesIndex)) {
      // m_logger->debug("series {} is hidden", seriesIndex);
      continue;
    }

    QPointer<AbstractSeries> series = m_series[seriesIndex];
    if (!series) {
      continue;
    }

    auto snapshotIt = m_snapshots.constFind(resolved.sourceId);
    if (snapshotIt == m_snapshots.constEnd()) {
      m_logger->info(
          "ChartView::rebuildXYSeriesRenderPackage: no snapshot found for "
          "sourceId {}, skipping",
          resolved.sourceId);
      continue;
    }

    const DataSnapshot &snapshot = snapshotIt.value();

    // CP_DEBUG("Strategy {} building data", seriesIndex);
    std::unique_ptr<RenderData> data = m_strategies[seriesIndex]->build(
        *series, resolved, snapshot, buildContext);
    // CP_DEBUG("Strategy {} done building data", seriesIndex);

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

    if (!xyData->yRange.valid) {
      globalY = xyData->yRange;
    } else {
      globalY = DataRangeCalculator::unionRange(globalY, xyData->yRange);
    }

    SeriesRenderPayload payload;
    payload.seriesIndex = seriesIndex;
    payload.data = std::move(data);

    package.seriesPayloads.push_back(std::move(payload));
  }

  if (package.seriesPayloads.empty()) {
    m_logger->info("ChartView::rebuildXYSeriesRenderPackage: no valid XY "
                   "series render payloads, rendering empty axes");
    globalX = DataRange{.min = 0, .max = 1, .valid = true};
    globalY = DataRange{.min = 0, .max = 1, .valid = true};
  }

  if (!globalX.valid || !globalY.valid) {
    m_logger->warn(
        "ChartView::rebuildXYSeriesRenderPackage: global XY range is invalid");
    return false;
  }

  if (m_isPanning && m_lockedYRange.valid) {
    globalY = m_lockedYRange;
  }

  // m_logger->debug("globalX = {}, globalY = {}",
  //                 globalX.toString().toStdString(),
  //                 globalY.toString().toStdString());

  // Special case: only 1 point
  if (globalX.min == globalX.max) {
    if (globalX.min == 0) {
      globalX.min = -1;
      globalX.max = 1;
    } else {
      globalX.min = globalX.min * 0.9;
      globalX.max = globalX.max * 1.1;
    }
  }
  if (globalY.min == globalY.max) {
    if (globalY.min == 0) {
      globalY.min = -1;
      globalY.max = 1;
    } else {
      globalY.min = globalY.min * 0.9;
      globalY.max = globalY.max * 1.1;
    }
  }

  m_plotContext.xRange = globalX;
  m_plotContext.yRange = globalY;

  const AxisModel xModel =
      xIsCategory
          ? AxisBuilder::buildCategoryAxis(resolvedResult.sharedXCategories)
          : AxisBuilder::buildValueAxis(globalX,
                                        resolvedResult.sharedXColumnType ==
                                            ChartEnums::DataType::Date,
                                        m_generalConfig.xPreferredTickCount());
  const AxisModel yModel = AxisBuilder::buildValueAxis(
      globalY, false, m_generalConfig.yPreferredTickCount());

  /**
   * X axis range will follow the visible range we want, while Y axis range
   * will depend on points that in visible X range (except when we are panning
   * then y range is locked).
   *
   * Basically, X axis range IS visible data range, Y axis range will be
   * calculated based on that.
   */
  m_plotContext.xAxisRange = AxisRange::fromDataRange(globalX);
  m_plotContext.yAxisRange = yModel.range;
  m_plotContext.axisPositions =
      ChartEnums::AxisPosition::Left | ChartEnums::AxisPosition::Bottom;

  package.xAxisPayload = AxisPayload{
      .data = AxisBuilder::toRenderData(
          xModel, ChartEnums::AxisPosition::Bottom, yModel.range.min,
          AxisRange::fromDataRange(globalX)),
  };
  package.yAxisPayload = AxisPayload{
      .data = AxisBuilder::toRenderData(yModel, ChartEnums::AxisPosition::Left,
                                        globalX.min, yModel.range),
  };

  m_pendingRenderPackage = std::move(package);

  // CP_DEBUG("End rebuilding XY Series Render package...");
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
  if (chartView->m_dataManagerPool) {
    chartView->m_dataManagerPool->shutdownDataManagers();
  }
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

void ChartView::relayout() {
  QRectF rect = boundingRect();

  if (m_titleItem) {
    if (m_plan.hasTitle) {
      m_titleItem->setVisible(true);
      const float h = m_titleItem->implicitHeight() > 0
                          ? m_titleItem->implicitHeight()
                          : m_titleItem->height();
      m_titleItem->setPosition(QPointF(rect.left(), rect.top()));
      m_titleItem->setSize(QSizeF(rect.width(), h));
      rect.setTop(rect.top() + h);
    } else {
      m_titleItem->setVisible(false);
    }
  }

  if (m_legendItem) {
    const auto pos = m_plan.legendPosition;
    m_legendItem->setProperty("horizontal",
                              pos == ChartEnums::LegendPosition::Top ||
                                  pos == ChartEnums::LegendPosition::Bottom);
    if (pos != ChartEnums::LegendPosition::None) {
      m_legendItem->setVisible(true);
      switch (pos) {
      case ChartEnums::LegendPosition::Right: {
        const qreal w = m_legendItem->implicitWidth();
        m_legendItem->setPosition(QPointF(rect.right() - w, rect.top()));
        m_legendItem->setSize(QSizeF(w, rect.height()));
        rect.setRight(rect.right() - w);
        break;
      }
      case ChartEnums::LegendPosition::Left: {
        const qreal w = m_legendItem->implicitWidth();
        m_legendItem->setPosition(QPointF(rect.left(), rect.top()));
        m_legendItem->setSize(QSizeF(w, rect.height()));
        rect.setLeft(rect.left() + w);
        break;
      }
      case ChartEnums::LegendPosition::Top: {
        const qreal h = m_legendItem->implicitHeight();
        m_legendItem->setPosition(QPointF(rect.left(), rect.top()));
        m_legendItem->setSize(QSizeF(rect.width(), h));
        rect.setTop(rect.top() + h);
        break;
      }
      case ChartEnums::LegendPosition::Bottom: {
        const qreal h = m_legendItem->implicitHeight();
        m_legendItem->setPosition(QPointF(rect.left(), rect.bottom() - h));
        m_legendItem->setSize(QSizeF(rect.width(), h));
        rect.setBottom(rect.bottom() - h);
        break;
      }
      default:
        break;
      }
    } else {
      m_legendItem->setVisible(false);
    }
  }

  m_plotOuterRect = rect;
  scheduleUpdate();
}

void ChartView::scheduleUpdate() {
  if (m_updatePending) {
    return;
  }

  m_updatePending = true;

  qint64 interval = 1000 / m_fps;
  qint64 elapsed = m_lastUpdateTimer.elapsed();

  if (elapsed >= interval) {
    performScheduledUpdate();
  } else {
    m_updateTimer->start(interval - elapsed);
  }
}

void ChartView::performScheduledUpdate() {
  m_updatePending = false;
  m_lastUpdateTimer.restart();

  if (m_plan.valid && rebuildRenderPackage()) {
    update();
  }
}

void ChartView::replan() {
  if (!isComponentComplete()) {
    return;
  }
  ChartLayoutPlanner planner;
  ChartChromeRequest chrome;
  chrome.hasTitle = !m_title.isEmpty();
  chrome.legendPosition = m_legendPosition;
  m_plan = planner.buildPlan(m_series, chrome);
  relayout();
}

void ChartView::rebuildLegendModel() {
  if (!m_legendModel) {
    return;
  }
  QVector<LegendEntry> entries;
  entries.reserve(m_series.size());
  for (int i = 0; i < m_series.size(); ++i) {
    AbstractSeries *series = m_series.at(i);
    LegendEntry entry;
    if (series) {
      entry.name = series->name();
      entry.color = series->legendColor();
    }
    if (entry.name.isEmpty()) {
      entry.name = QStringLiteral("Series %1").arg(i + 1);
    }
    entries.push_back(entry);
  }
  m_legendModel->setEntries(entries);
}

QString ChartView::title() const { return m_title; }
void ChartView::setTitle(const QString &title) {
  if (m_title == title) {
    return;
  }
  m_title = title;
  emit titleChanged();
  replan(); // hasTitle depends on whether a title is set
}

ChartEnums::LegendPosition ChartView::legendPosition() const {
  return m_legendPosition;
}
void ChartView::setLegendPosition(ChartEnums::LegendPosition position) {
  if (m_legendPosition == position) {
    return;
  }
  m_legendPosition = position;
  emit legendPositionChanged();
  replan();
}

LegendModel *ChartView::legendModel() const { return m_legendModel; }

QQuickItem *ChartView::titleItem() const { return m_titleItem; }
void ChartView::setTitleItem(QQuickItem *item) {
  if (m_titleItem == item) {
    return;
  }
  m_titleItem = item;
  if (item) {
    item->setParentItem(this);
  }
  emit titleItemChanged();
  relayout();
}

QQuickItem *ChartView::legendItem() const { return m_legendItem; }
void ChartView::setLegendItem(QQuickItem *item) {
  if (m_legendItem == item) {
    return;
  }
  if (m_legendItem) {
    m_legendItem->disconnect(this);
  }
  m_legendItem = item;
  if (item) {
    item->setParentItem(this);
    connect(item, &QQuickItem::implicitWidthChanged, this,
            &ChartView::relayout);
    connect(item, &QQuickItem::implicitHeightChanged, this,
            &ChartView::relayout);
  }
  emit legendItemChanged();
  relayout();
}

QVariantList ChartView::seriesList() const {
  QVariantList list;
  list.reserve(m_series.size());
  for (const auto &s : m_series) {
    list.append(QVariant::fromValue(static_cast<QObject *>(s.data())));
  }
  return list;
}

void ChartView::applySettings(float globalStrokeWidth, float globalAntialiasing,
                              int xPreferredTickCount,
                              int yPreferredTickCount) {
  m_generalConfig.setLineWidth(globalStrokeWidth);
  m_generalConfig.setAntialiasing(globalAntialiasing);
  m_generalConfig.setXPreferredTickCount(xPreferredTickCount);
  m_generalConfig.setYPreferredTickCount(yPreferredTickCount);

  emit generalConfigChanged();

  if (m_plan.valid && rebuildRenderPackage()) {
    // user can change line color and make legend changes
    rebuildLegendModel();
    scheduleUpdate();
  }
}

} // namespace ChartPlotter

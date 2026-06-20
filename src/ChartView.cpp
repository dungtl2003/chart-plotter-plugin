#include "ChartPlotter/ChartView.hpp"

#include "ChartPlotter/axis/AxisBuilder.hpp"
#include "ChartPlotter/data/RenderData.hpp"
#include "ChartPlotter/node/ChartRenderNode.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"
#include "factory/SeriesComponentFactoryProvider.hpp"

namespace ChartPlotter {

float GeneralConfig::lineWidth() const { return m_lineWidth; }
void GeneralConfig::setLineWidth(float newLineWidth) {
  m_lineWidth = newLineWidth;
}

float GeneralConfig::antialiasing() const { return m_antialiasing; }
void GeneralConfig::setAntialiasing(float a) { m_antialiasing = a; }

bool GeneralConfig::operator==(const GeneralConfig &o) const {
  return m_lineWidth == o.m_lineWidth && m_antialiasing == o.m_antialiasing;
}
bool GeneralConfig::operator!=(const GeneralConfig &other) const {
  return m_lineWidth != other.lineWidth() ||
         m_antialiasing != other.m_antialiasing;
}

ChartView::ChartView(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);

  m_dataManagerPool = new DataManagerPool(this);
  m_legendModel = new LegendModel(this);
  m_name = QString::fromStdString(appendUniqueId("ChartView_EarlyInit_"));
  m_logger = LoggerManager::createInstanceLogger(m_name.toStdString());
  connect(m_dataManagerPool, &DataManagerPool::errorOccurred, this,
          &ChartView::onDataError, Qt::QueuedConnection);
  connect(
      m_dataManagerPool, &DataManagerPool::snapshotReady, this,
      [this](int id, const DataSnapshot &snapshot) {
        onSnapshotReady(id, snapshot);
      },
      Qt::QueuedConnection);

  connect(m_legendModel, &LegendModel::visibilityChanged, this,
          [this](int, bool) {
            if (m_plan.valid && rebuildRenderPackage()) {
              update();
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
}

void ChartView::geometryChange(const QRectF &newGeom, const QRectF &oldGeom) {
  QQuickItem::geometryChange(newGeom, oldGeom);
  relayout();
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

  m_resolvedSeries = m_seriesDataResolver.resolve(
      m_plan.xySeriesIndexes, m_plan.pieSeriesIndexes, m_series,
      m_dataManagerPool->sourceIds(), m_snapshots);

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

  const bool xIsCategory =
      resolvedResult.sharedXColumnType == ChartEnums::DataType::String;

  SeriesBuildContext buildContext;
  buildContext.xCategories =
      xIsCategory ? &resolvedResult.sharedXCategories : nullptr;
  buildContext.globalLineWidth = m_generalConfig.lineWidth();
  buildContext.globalAntialiasing = m_generalConfig.antialiasing();

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

  const AxisModel xModel =
      xIsCategory
          ? AxisBuilder::buildCategoryAxis(resolvedResult.sharedXCategories)
          : AxisBuilder::buildValueAxis(globalX,
                                        resolvedResult.sharedXColumnType ==
                                            ChartEnums::DataType::Date,
                                        6);

  const AxisModel yModel = AxisBuilder::buildValueAxis(globalY);

  m_plotContext.xAxisRange = xModel.range;
  m_plotContext.yAxisRange = yModel.range;
  m_plotContext.axisPositions =
      ChartEnums::AxisPosition::Left | ChartEnums::AxisPosition::Bottom;

  package.xAxisPayload = AxisPayload{
      .data = AxisBuilder::toRenderData(
          xModel, ChartEnums::AxisPosition::Bottom, yModel.range.min),
  };
  package.yAxisPayload = AxisPayload{
      .data = AxisBuilder::toRenderData(yModel, ChartEnums::AxisPosition::Left,
                                        xModel.range.min),
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
      const qreal h = m_titleItem->implicitHeight() > 0
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
  update();
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

void ChartView::applySettings(qreal globalStrokeWidth,
                              qreal globalAntialiasing) {
  m_generalConfig.setLineWidth(static_cast<float>(globalStrokeWidth));
  m_generalConfig.setAntialiasing(static_cast<float>(globalAntialiasing));
  emit generalConfigChanged();
  if (m_plan.valid && rebuildRenderPackage()) {
    update();
  }
}

} // namespace ChartPlotter

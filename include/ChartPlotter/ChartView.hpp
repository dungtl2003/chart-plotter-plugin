#pragma once

#include "ChartPlotter/ChartLayoutPlanner.hpp"
#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/data/DataManager.hpp"
#include "ChartPlotter/data/DataSource.hpp"
#include "ChartPlotter/data/RenderData.hpp"
#include "ChartPlotter/renderer/IOpenGLRenderer.hpp"
#include "ChartPlotter/series/AbstractSeries.hpp"
#include "ChartPlotter/series/SeriesDataResolver.hpp"
#include "ChartPlotter/strategy/ISeriesStrategy.hpp"

#include <spdlog/spdlog.h>

#include <QQuickItem>
#include <QtQml>
#include <memory>

namespace ChartPlotter {

struct DataManagerRuntime {
  int id = -1;
  QPointer<QThread> thread = nullptr;
  QPointer<DataManager> manager = nullptr;
};

class ChartView : public QQuickItem {
  Q_OBJECT
  QML_ELEMENT
  Q_CLASSINFO("DefaultProperty", "content")

  Q_PROPERTY(QQmlListProperty<QObject> content READ content)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
  using RendererCreator = std::function<std::unique_ptr<IOpenGLRenderer>()>;

  explicit ChartView(QQuickItem *parent = nullptr);
  explicit ChartView(const ChartView &) = delete;
  explicit ChartView(ChartView &&) = delete;
  ChartView &operator=(const ChartView &) = delete;
  ChartView &operator=(ChartView &&) = delete;
  ~ChartView();

  QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
  void componentComplete() override;

  QQmlListProperty<QObject> content();
  QString name() const;
  void setName(QString newName);

public slots:
  void onDataError(const QString &message);
  void onSnapshotReady(int sourceId, const DataSnapshot &snapshot);

signals:
  void nameChanged();
  void stopDataManagerRequested(int id);

private:
  QHash<DataSource *, int> m_sourceIds;
  QHash<int, DataSnapshot> m_snapshots;
  QList<QPointer<QObject>> m_content;
  QVector<QPointer<AbstractSeries>> m_series;
  QVector<QPointer<DataSource>> m_sources;
  QVector<DataManagerRuntime> m_dataManagers;
  QString m_name;
  int m_nextSourceId = 0;
  std::vector<std::unique_ptr<ISeriesStrategy>> m_strategies;
  std::shared_ptr<spdlog::logger> m_logger;
  ChartLayoutPlan m_plan;
  SeriesDataResolver m_seriesDataResolver;
  SeriesResolveResult m_resolvedSeries;
  std::optional<ChartRenderPackage> m_pendingRenderPackage;

  void dropLogger();
  std::string appendUniqueId(std::string s) const;
  void resetStrategies();
  QPointer<DataManager> createDataManager(const QPointer<DataSource> source);
  void stopDataManager(int id);
  void shutdownDataManagers();
  void shutdownDataManager(QPointer<DataManager> manager,
                           QPointer<QThread> thread);
  bool rebuildRenderPackage();
  bool rebuildXYSeriesRenderPackage(const SeriesResolveResult &resolvedSeries);
  bool rebuildPieSeriesRenderPackage(const SeriesResolveResult &resolvedSeries);
  DataRange unionRange(const DataRange &a, const DataRange &b) const;
  std::vector<std::unique_ptr<IOpenGLRenderer>> createRenderersFromPlan() const;
  static void appendContent(QQmlListProperty<QObject> *property,
                            QObject *object);
  static qsizetype contentCount(QQmlListProperty<QObject> *property);
  static QObject *contentAt(QQmlListProperty<QObject> *property,
                            qsizetype index);
  static void clearContent(QQmlListProperty<QObject> *property);
};

} // namespace ChartPlotter

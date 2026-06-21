#pragma once

#include "ChartPlotter/ChartLayoutPlanner.hpp"
#include "ChartPlotter/ViewportController.hpp"
#include "ChartPlotter/data/DataBuffer.hpp"
#include "ChartPlotter/data/DataManagerPool.hpp"
#include "ChartPlotter/data/DataSource.hpp"
#include "ChartPlotter/data/RenderData.hpp"
#include "ChartPlotter/legend/LegendModel.hpp"
#include "ChartPlotter/renderer/IOpenGLRenderer.hpp"
#include "ChartPlotter/series/AbstractSeries.hpp"
#include "ChartPlotter/series/SeriesDataResolver.hpp"
#include "ChartPlotter/strategy/ISeriesStrategy.hpp"

#include <spdlog/spdlog.h>

#include <QQuickItem>
#include <QtQml>
#include <memory>

namespace ChartPlotter {

class GeneralConfig {
  Q_GADGET

  Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth)
  Q_PROPERTY(float antialiasing READ antialiasing WRITE setAntialiasing)

public:
  bool operator==(const GeneralConfig &other) const;
  bool operator!=(const GeneralConfig &other) const;

  float lineWidth() const;
  void setLineWidth(float newWidth);

  float antialiasing() const;
  void setAntialiasing(float a);

private:
  float m_lineWidth = 5.0f;
  float m_antialiasing = 1.0f;
};

class ChartView : public QQuickItem {
  Q_OBJECT
  QML_ELEMENT
  Q_CLASSINFO("DefaultProperty", "content")

  Q_PROPERTY(QQmlListProperty<QObject> content READ content)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY(ChartPlotter::GeneralConfig generalConfig READ generalConfig WRITE
                 setGeneralConfig NOTIFY generalConfigChanged)

  Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
  Q_PROPERTY(
      ChartPlotter::ChartEnums::LegendPosition legendPosition READ
          legendPosition WRITE setLegendPosition NOTIFY legendPositionChanged)
  Q_PROPERTY(ChartPlotter::LegendModel *legendModel READ legendModel CONSTANT)
  Q_PROPERTY(QQuickItem *titleItem READ titleItem WRITE setTitleItem NOTIFY
                 titleItemChanged)
  Q_PROPERTY(QQuickItem *legendItem READ legendItem WRITE setLegendItem NOTIFY
                 legendItemChanged)

  Q_PROPERTY(QVariantList seriesList READ seriesList NOTIFY seriesListChanged)

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

  GeneralConfig generalConfig() const;
  void setGeneralConfig(const GeneralConfig &newConfig);

  QString title() const;
  void setTitle(const QString &title);

  ChartEnums::LegendPosition legendPosition() const;
  void setLegendPosition(ChartEnums::LegendPosition position);

  LegendModel *legendModel() const;

  QQuickItem *titleItem() const;
  void setTitleItem(QQuickItem *item);
  QQuickItem *legendItem() const;
  void setLegendItem(QQuickItem *item);

  QVariantList seriesList() const;

  Q_INVOKABLE void applySettings(qreal globalStrokeWidth,
                                 qreal globalAntialiasing);

public slots:
  void onDataError(const QString &message);
  void onSnapshotReady(int sourceId, const DataSnapshot &snapshot);

protected:
  void geometryChange(const QRectF &newGeom, const QRectF &oldGeom) override;
  void wheelEvent(QWheelEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

signals:
  void nameChanged();
  void generalConfigChanged();
  void titleChanged();
  void legendPositionChanged();
  void titleItemChanged();
  void legendItemChanged();
  void seriesListChanged();

private:
  QPointer<DataManagerPool> m_dataManagerPool = nullptr;
  QHash<int, DataSnapshot> m_snapshots;
  QList<QPointer<QObject>> m_content;
  QVector<QPointer<AbstractSeries>> m_series;
  QVector<QPointer<DataSource>> m_sources;
  QPointer<ViewportController> m_viewportController;
  QString m_name;
  GeneralConfig m_generalConfig;
  std::vector<std::unique_ptr<ISeriesStrategy>> m_strategies;
  std::shared_ptr<spdlog::logger> m_logger;
  ChartLayoutPlan m_plan;
  SeriesDataResolver m_seriesDataResolver;
  SeriesResolveResult m_resolvedSeries;
  std::optional<ChartRenderPackage> m_pendingRenderPackage;
  PlotContext m_plotContext;
  QString m_title;
  ChartEnums::LegendPosition m_legendPosition =
      ChartEnums::LegendPosition::Right;
  LegendModel *m_legendModel = nullptr;
  QPointer<QQuickItem> m_titleItem;
  QPointer<QQuickItem> m_legendItem;
  QRectF m_plotOuterRect;

  bool m_isPanning = false;
  QPointF m_panLastMousePos;
  DataRange m_lockedYRange;

  void dropLogger();
  std::string appendUniqueId(std::string s) const;
  void resetStrategies();
  bool rebuildRenderPackage();
  bool rebuildXYSeriesRenderPackage(const SeriesResolveResult &resolvedSeries);
  bool rebuildPieSeriesRenderPackage(const SeriesResolveResult &resolvedSeries);
  std::vector<std::unique_ptr<IOpenGLRenderer>> createRenderersFromPlan() const;
  static void appendContent(QQmlListProperty<QObject> *property,
                            QObject *object);
  static qsizetype contentCount(QQmlListProperty<QObject> *property);
  static QObject *contentAt(QQmlListProperty<QObject> *property,
                            qsizetype index);
  static void clearContent(QQmlListProperty<QObject> *property);

  void replan();
  void relayout();
  void rebuildLegendModel();
};

} // namespace ChartPlotter

Q_DECLARE_METATYPE(ChartPlotter::GeneralConfig)

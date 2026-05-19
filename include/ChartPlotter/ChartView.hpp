#pragma once

#include "ChartPlotter/chart/AbstractChart.hpp"
#include "ChartPlotter/factory/ChartComponentFactoryProvider.hpp"
#include "ChartPlotter/renderer/IChartRenderer.hpp"
#include "ChartPlotter/strategy/IChartStrategy.hpp"

#include <spdlog/spdlog.h>

#include <QQuickPaintedItem>
#include <QtQml>
#include <memory>

class ChartView : public QQuickPaintedItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(AbstractChart *chart READ chart WRITE setChart NOTIFY chartChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  Q_CLASSINFO("DefaultProperty", "chart")

public:
  explicit ChartView(QQuickItem *parent = nullptr);
  ~ChartView();

  void paint(QPainter *painter) override;

  AbstractChart *chart() const;
  void setChart(AbstractChart *chart);
  QColor color() const;
  void setColor(const QColor &color);
  QString name() const;
  void setName(const QString &name);

signals:
  void chartChanged();
  void colorChanged();
  void nameChanged();

private:
  AbstractChart *m_chart;
  std::unique_ptr<IChartStrategy> m_strategy;
  std::unique_ptr<IChartRenderer> m_renderer;
  QColor m_color;
  std::shared_ptr<spdlog::logger> m_logger;
  QString m_name;

  void dropLogger();
};

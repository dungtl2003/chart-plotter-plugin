#include "ChartPlotter/ChartView.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

#include <QPainter>
#include <sstream>

ChartView::ChartView(QQuickItem *parent) : QQuickPaintedItem(parent) {
  std::stringstream ss;
  ss << "ChartView_EarlyInit_" << this;

  this->m_logger = LoggerManager::createInstanceLogger(ss.str());
}

ChartView::~ChartView() { this->dropLogger(); }

QString ChartView::name() const { return this->m_name; }

void ChartView::setName(const QString &newName) {
  if (this->m_name == newName) {
    return;
  }

  this->m_name = newName;
  emit nameChanged();

  this->dropLogger();
  this->m_logger =
      LoggerManager::createInstanceLogger("ChartView_" + m_name.toStdString());

  this->m_logger->debug("Changed logger");
}

void ChartView::paint(QPainter *painter) {
  painter->save();

  QPen pen = painter->pen();
  pen.setBrush(this->m_color);
  pen.setWidth(3);
  painter->setPen(pen);
  const QRectF paintRect =
      QRectF({}, QSizeF{width(), height()}).adjusted(1, 1, -1, -1);
  painter->drawEllipse(paintRect);
  painter->restore();
}

QColor ChartView::color() const { return this->m_color; }

void ChartView::setColor(const QColor &newColor) {
  if (this->m_color == newColor) {
    return;
  }

  this->m_color = newColor;
  emit colorChanged();

  update();
}

AbstractChart *ChartView::chart() const { return this->m_chart; }

void ChartView::setChart(AbstractChart *chart) {
  if (this->m_chart == chart) {
    return;
  }

  this->m_chart = chart;

  if (!this->m_chart) {
    this->m_strategy.reset();
    this->m_renderer.reset();
    emit chartChanged();
    return;
  }

  auto factory = ChartComponentFactoryProvider::getFactory(chart->type());
  if (!factory) {
    this->m_logger->warn("Unsupported chart type");
    return;
  }

  this->m_strategy = factory->getStrategy();
  this->m_renderer = factory->getRenderer();

  emit chartChanged();

  this->m_strategy->calculate();
  this->m_renderer->render();

  update();
}

void ChartView::dropLogger() {
  if (!this->m_logger) {
    return;
  }

  std::string old_logger_name = m_logger->name();
  m_logger.reset();
  spdlog::drop(old_logger_name);
}

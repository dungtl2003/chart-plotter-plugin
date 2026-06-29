#pragma once

#include "ChartPlotter/series/XYSeries.hpp"

#include <QColor>

namespace ChartPlotter {

class BarSeries : public XYSeries {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
  explicit BarSeries(QObject *parent = nullptr);

  ChartEnums::SeriesType type() const override;
  QColor legendColor() const override;

  QColor color() const;
  void setColor(const QColor &color);

signals:
  void colorChanged();

private:
  QColor m_color = QColor("#3d7eff");
};

} // namespace ChartPlotter

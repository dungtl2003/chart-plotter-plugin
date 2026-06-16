#pragma once

#include "ChartPlotter/data/RenderData.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"

#include <QColor>

namespace ChartPlotter {

struct ValueAxisTickRenderData {
  QPointF pos;
  QString label;

  QString toString() const {
    return QString("Tick{pos = (%1, %2), label = '%3'}")
        .arg(pos.x())
        .arg(pos.y())
        .arg(label);
  }
};

struct ValueAxisRenderData : public RenderData {
  QVector<ValueAxisTickRenderData> ticks;
  ChartEnums::AxisPosition pos = ChartEnums::AxisPosition::Left;
  QColor color = QColor("#c1c1c1");
  double width = 5.0f;
  int fontSize = 20;

  QString toString() const override {
    QString result;
    result.reserve(ticks.size() * 50);

    for (size_t i = 0; i < ticks.size(); ++i) {
      result.append(ticks.at(i).toString());
      if (i < ticks.size() - 1) {
        result.append(", ");
      }
    }

    return QString("ValueAxisRenderData({ticks = [%1]\n})").arg(result);
  }
};

} // namespace ChartPlotter

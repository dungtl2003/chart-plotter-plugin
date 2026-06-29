#include "ChartPlotter/axis/AxisBuilder.hpp"
#include "ChartPlotter/axis/ValueAxis.hpp"

namespace ChartPlotter {

AxisModel AxisBuilder::buildValueAxis(const DataRange &dataRange, bool isDate,
                                      std::optional<int> tickCount) {
  AxisModel model;

  const AxisRange base = ValueAxis::calculateRange(dataRange);
  const AxisTicks ticks =
      tickCount ? ValueAxis::calculateTicks(base, *tickCount, isDate)
                : ValueAxis::calculateTicks(base);

  model.range = ticks.ticks.isEmpty()
                    ? base
                    : AxisRange{.min = ticks.ticks.first().value,
                                .max = ticks.ticks.last().value};

  model.ticks.reserve(ticks.ticks.size());
  for (const AxisTick &t : ticks.ticks) {
    // Use the tick's own value, not min + i*step: the latter assumes
    // perfectly uniform spacing anchored at the range min, which is wrong for
    // non-uniform (e.g. date) ticks.
    // if (t.value >= dataRange.min && t.value <= dataRange.max) {
    //   model.ticks.push_back({t.value, t.label});
    // }
    model.ticks.push_back({t.value, t.label});
  }

  return model;
}

AxisModel AxisBuilder::buildCategoryAxis(const CategoryAxis &categories) {
  AxisModel model;

  const int n = categories.size();
  // Bar/category axes render in BetweenTicks mode: the ticks are the band
  // *boundaries* at {-0.5, 0.5, ..., n-0.5} and each category label is centered
  // in the gap between two boundaries. Category i is centered on integer i, so a
  // bar (or point) at i sits in the middle of its band instead of on a tick.
  model.range = AxisRange{.min = -0.5, .max = static_cast<double>(n) - 0.5};

  model.ticks.reserve(n + 1);
  for (int i = 0; i < n; ++i) {
    // Boundary i carries the label for the band to its right (category i).
    model.ticks.push_back({static_cast<double>(i) - 0.5, categories.labelAt(i)});
  }
  // Closing boundary — position only, its label is never used.
  model.ticks.push_back({static_cast<double>(n) - 0.5, QString()});

  return model;
}

std::unique_ptr<AxisRenderData>
AxisBuilder::toRenderData(const AxisModel &model, ChartEnums::AxisPosition pos,
                          double baseline, const AxisRange &range,
                          ChartEnums::TickMode tickMode) {
  auto data = std::make_unique<AxisRenderData>();
  data->pos = pos;
  data->tickMode = tickMode;

  const bool vertical = pos == ChartEnums::AxisPosition::Left ||
                        pos == ChartEnums::AxisPosition::Right;

  data->minPointInRange =
      vertical ? QPointF(baseline, range.min) : QPointF(range.min, baseline);
  data->maxPointInRange =
      vertical ? QPointF(baseline, range.max) : QPointF(range.max, baseline);

  data->ticks.reserve(model.ticks.size());
  for (const AxisTick &t : model.ticks) {
    // Vertical axes vary along Y at fixed X (=baseline); horizontal axes vary
    // along X at fixed Y (=baseline).
    if (t.value < range.min || t.value > range.max) {
      continue;
    }
    const QPointF p =
        vertical ? QPointF(baseline, t.value) : QPointF(t.value, baseline);
    data->ticks.push_back(AxisTickRenderData{.pos = p, .label = t.label});
  }

  return data;
}

} // namespace ChartPlotter

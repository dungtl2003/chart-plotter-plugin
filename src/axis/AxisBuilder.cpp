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
    model.ticks.push_back({t.value, t.label});
  }

  return model;
}

AxisModel AxisBuilder::buildCategoryAxis(const CategoryAxis &categories) {
  AxisModel model;

  const int n = categories.size();
  // {0, n-1} keeps end categories flush with the frame; half-step padding
  // would be {-0.5, n-0.5}.
  model.range = AxisRange{.min = 0, .max = static_cast<double>(n - 1)};

  model.ticks.reserve(n);
  for (int i = 0; i < n; ++i) {
    model.ticks.push_back({static_cast<double>(i), categories.labelAt(i)});
  }

  return model;
}

std::unique_ptr<AxisRenderData>
AxisBuilder::toRenderData(const AxisModel &model, ChartEnums::AxisPosition pos,
                          double baseline) {
  auto data = std::make_unique<AxisRenderData>();
  data->pos = pos;

  const bool vertical = pos == ChartEnums::AxisPosition::Left ||
                        pos == ChartEnums::AxisPosition::Right;

  data->ticks.reserve(model.ticks.size());
  for (const AxisTick &t : model.ticks) {
    // Vertical axes vary along Y at fixed X (=baseline); horizontal axes vary
    // along X at fixed Y (=baseline).
    const QPointF p =
        vertical ? QPointF(baseline, t.value) : QPointF(t.value, baseline);
    data->ticks.push_back(AxisTickRenderData{.pos = p, .label = t.label});
  }

  return data;
}

} // namespace ChartPlotter

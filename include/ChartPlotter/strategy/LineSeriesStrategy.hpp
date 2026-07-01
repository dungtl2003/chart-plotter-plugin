#pragma once

#include "ChartPlotter/data/LineRenderData.hpp"
#include "ChartPlotter/strategy/ISeriesStrategy.hpp"

#include <deque>
#include <expected>

namespace ChartPlotter {

class LineSeriesStrategy : public ISeriesStrategy {
public:
  std::unique_ptr<RenderData> build(const AbstractSeries &series,
                                    const ResolvedSeriesData &resolved,
                                    const DataSnapshot &snapshot,
                                    SeriesBuildContext &context) override;

private:
  std::expected<void, QString> convertRowsToPoints(
      QVector<QPointF> &destination, const AbstractSeries &series,
      const ResolvedSeriesData &resolved, const DataSnapshot &snapshot,
      const SeriesBuildContext &context, qsizetype fromRow,
      qsizetype endRow) const;
  std::expected<void, QString>
  loadSeriesConfig(LineRenderData *data, const AbstractSeries &series,
                   const SeriesBuildContext &context) const;
  std::pair<std::deque<QPointF>::const_iterator,
            std::deque<QPointF>::const_iterator>
  calculateBound(const std::deque<QPointF> &points,
                 const SeriesBuildContext &context) const;
  std::expected<void, QString>
  validateTyAndColIndex(const ResolvedSeriesData &resolved,
                        const DataSnapshot &snapshot) const;
  // x of the earliest live (non-NaN) row = lower edge of the current window.
  // Used to evict cached points that fall before it. Returns false if no live
  // row has a valid x.
  bool firstLiveXValue(const DataSnapshot &snapshot,
                       const ResolvedSeriesData &resolved, double &out) const;
};

} // namespace ChartPlotter

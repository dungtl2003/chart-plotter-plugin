#pragma once

#include "ChartPlotter/data/LineRenderData.hpp"
#include "ChartPlotter/strategy/ISeriesStrategy.hpp"

#include <expected>

namespace ChartPlotter {

class LineSeriesStrategy : public ISeriesStrategy {
public:
  std::unique_ptr<RenderData> build(const AbstractSeries &series,
                                    const ResolvedSeriesData &resolved,
                                    const DataSnapshot &snapshot,
                                    const SeriesBuildContext &context) override;

private:
  std::expected<void, QString> convertRowsToPoints(
      QVector<QPointF>::iterator destinationIt, const AbstractSeries &series,
      const ResolvedSeriesData &resolved, const DataSnapshot &snapshot,
      const SeriesBuildContext &context, qsizetype fromRow) const;
  std::expected<void, QString>
  loadSeriesConfig(LineRenderData *data, const AbstractSeries &series,
                   const SeriesBuildContext &context) const;
  std::pair<QVector<QPointF>::const_iterator, QVector<QPointF>::const_iterator>
  calculateBound(const QVector<QPointF> points,
                 const SeriesBuildContext &context) const;
};

} // namespace ChartPlotter

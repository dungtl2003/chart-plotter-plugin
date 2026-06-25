#pragma once

#include "ChartPlotter/downsample/DataDownsampler.hpp"

namespace ChartPlotter {

class HybridDownsampler : public DataDownsampler {
public:
  void downsample(QVector<QPointF>::const_iterator source, qsizetype sourceSize,
                  QVector<QPointF>::iterator destination,
                  qsizetype destinationSize) const override;

private:
  const double LTTB_BUCKET_SIZE_THRES = 500.0;

  void dispatchConcurrentMinMax(QVector<QPointF>::const_iterator source,
                                qsizetype sourceSize,
                                QVector<QPointF>::iterator destination,
                                qsizetype destinationSize) const;
  void dispatchConcurrentAvg(QVector<QPointF>::const_iterator source,
                             qsizetype sourceSize,
                             QVector<QPointF>::iterator destination,
                             qsizetype destinationSize) const;
  void dispatchConcurrentLTTB(QVector<QPointF>::const_iterator source,
                              qsizetype sourceSize,
                              QVector<QPointF>::iterator destination,
                              qsizetype destinationSize) const;
};

} // namespace ChartPlotter

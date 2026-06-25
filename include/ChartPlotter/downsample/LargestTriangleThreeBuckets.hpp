#pragma once

#include "ChartPlotter/downsample/DataDownsampler.hpp"

namespace ChartPlotter {

class LargestTriangleThreeBuckets : public DataDownsampler {
public:
  void downsample(QVector<QPointF>::const_iterator source, qsizetype sourceSize,
                  QVector<QPointF>::iterator destination,
                  qsizetype destinationSize) const override;
};

} // namespace ChartPlotter

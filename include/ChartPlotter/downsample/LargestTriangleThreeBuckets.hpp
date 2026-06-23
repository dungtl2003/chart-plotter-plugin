#pragma once

#include "ChartPlotter/downsample/DataDownsampler.hpp"

namespace ChartPlotter {

class LargestTriangleThreeBuckets : public DataDownsampler {
  void downsample(QVector<QPointF>::iterator source, qsizetype sourceSize,
                  QVector<QPointF>::iterator destination,
                  qsizetype destinationSize) override;
};

} // namespace ChartPlotter

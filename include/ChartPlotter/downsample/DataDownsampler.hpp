#pragma once

#include <QPointF>
#include <QVector>

namespace ChartPlotter {

class DataDownsampler {
public:
  DataDownsampler() = default;

  explicit DataDownsampler(const DataDownsampler &) = delete;
  explicit DataDownsampler(DataDownsampler &&) = delete;
  DataDownsampler &operator=(const DataDownsampler &) = delete;
  DataDownsampler &operator=(DataDownsampler &&) = delete;
  virtual ~DataDownsampler() = default;

  virtual void downsample(QVector<QPointF>::iterator source,
                          qsizetype sourceSize,
                          QVector<QPointF>::iterator destination,
                          qsizetype destinationSize) = 0;
};

} // namespace ChartPlotter

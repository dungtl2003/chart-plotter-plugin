#include "ChartPlotter/downsample/HybridDownsampler.hpp"

#include <QFutureSynchronizer>
#include <QThread>
#include <QtConcurrent>
#include <cmath>
#include <limits>

namespace ChartPlotter {

void HybridDownsampler::downsample(QVector<QPointF>::const_iterator source,
                                   qsizetype sourceSize,
                                   QVector<QPointF>::iterator destination,
                                   qsizetype destinationSize) const {
  if (destinationSize <= 0 || sourceSize <= 0) {
    return;
  }

  if (destinationSize >= sourceSize) {
    std::copy_n(source, sourceSize, destination);
    return;
  }

  if (destinationSize == 1) {
    *destination = *source;
    return;
  }

  double downsampleRatio =
      static_cast<double>(sourceSize) / static_cast<double>(destinationSize);

  if (downsampleRatio > LTTB_BUCKET_SIZE_THRES) {
    dispatchConcurrentAvg(source, sourceSize, destination, destinationSize);
  } else {
    dispatchConcurrentLTTB(source, sourceSize, destination, destinationSize);
  }
}

void HybridDownsampler::dispatchConcurrentMinMax(
    QVector<QPointF>::const_iterator source, qsizetype sourceSize,
    QVector<QPointF>::iterator destination, qsizetype destinationSize) const {

  destination[0] = source[0];
  destination[destinationSize - 1] = source[sourceSize - 1];

  qsizetype totalInnerSlots = destinationSize - 2;

  // Min-Max requires 2 points per bucket. So we need half the buckets.
  qsizetype numBuckets = totalInnerSlots / 2;
  if (numBuckets == 0) {
    return;
  }

  double bucketSize =
      static_cast<double>(sourceSize - 2) / static_cast<double>(numBuckets);

  int threadCount = QThread::idealThreadCount();
  if (threadCount < 1) {
    threadCount = 1;
  }

  qsizetype bucketsPerThread = numBuckets / threadCount;
  QFutureSynchronizer<void> synchronizer;

  for (int t = 0; t < threadCount; ++t) {
    qsizetype startBucket = t * bucketsPerThread;
    qsizetype endBucket =
        (t == threadCount - 1) ? numBuckets : startBucket + bucketsPerThread;

    if (startBucket >= endBucket) {
      continue;
    }

    synchronizer.addFuture(QtConcurrent::run([=]() {
      for (qsizetype i = startBucket; i < endBucket; ++i) {
        qsizetype rangeFrom = static_cast<qsizetype>(i * bucketSize) + 1;
        qsizetype rangeTo = static_cast<qsizetype>((i + 1) * bucketSize) + 1;
        if (rangeTo >= sourceSize - 1) {
          rangeTo = sourceSize - 1;
        }

        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();

        // Grab the first X to act as the vertical bar's position
        double chunkX = source[rangeFrom].x();

        for (qsizetype j = rangeFrom; j < rangeTo; ++j) {
          double y = source[j].y();
          if (y < minY) {
            minY = y;
          }
          if (y > maxY) {
            maxY = y;
          }
        }

        destination[1 + (i * 2)] = QPointF(chunkX, minY);
        destination[1 + (i * 2) + 1] = QPointF(chunkX, maxY);
      }
    }));
  }

  synchronizer.waitForFinished();

  // If the user requested an odd number of total points (e.g., 1001),
  // we have 1 unwritten slot at the end. We duplicate the last inner point to
  // fill it safely.
  if (totalInnerSlots % 2 != 0) {
    destination[destinationSize - 2] = destination[destinationSize - 3];
  }
}

void HybridDownsampler::dispatchConcurrentAvg(
    QVector<QPointF>::const_iterator source, qsizetype sourceSize,
    QVector<QPointF>::iterator destination, qsizetype destinationSize) const {

  destination[0] = source[0];
  destination[destinationSize - 1] = source[sourceSize - 1];

  qsizetype totalInnerSlots = destinationSize - 2;

  qsizetype numBuckets = totalInnerSlots;
  if (numBuckets == 0) {
    return;
  }

  double bucketSize =
      static_cast<double>(sourceSize - 2) / static_cast<double>(numBuckets);

  int threadCount = QThread::idealThreadCount();
  if (threadCount < 1) {
    threadCount = 1;
  }

  qsizetype bucketsPerThread = numBuckets / threadCount;
  QFutureSynchronizer<void> synchronizer;

  for (int t = 0; t < threadCount; ++t) {
    qsizetype startBucket = t * bucketsPerThread;
    qsizetype endBucket =
        (t == threadCount - 1) ? numBuckets : startBucket + bucketsPerThread;

    if (startBucket >= endBucket) {
      continue;
    }

    synchronizer.addFuture(QtConcurrent::run([=]() {
      for (qsizetype i = startBucket; i < endBucket; ++i) {
        qsizetype rangeFrom = static_cast<qsizetype>(i * bucketSize) + 1;
        qsizetype rangeTo = static_cast<qsizetype>((i + 1) * bucketSize) + 1;
        if (rangeTo >= sourceSize - 1) {
          rangeTo = sourceSize - 1;
        }

        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();

        // Grab the first X to act as the vertical bar's position
        double chunkX = source[rangeFrom].x();

        for (qsizetype j = rangeFrom; j < rangeTo; ++j) {
          double y = source[j].y();
          if (y < minY) {
            minY = y;
          }
          if (y > maxY) {
            maxY = y;
          }
        }

        destination[i + 1] = QPointF(chunkX, (maxY + minY) / 2.0);
      }
    }));
  }

  synchronizer.waitForFinished();
}

void HybridDownsampler::dispatchConcurrentLTTB(
    QVector<QPointF>::const_iterator source, qsizetype sourceSize,
    QVector<QPointF>::iterator destination, qsizetype destinationSize) const {

  destination[0] = source[0];
  destination[destinationSize - 1] = source[sourceSize - 1];

  qsizetype totalInnerBuckets = destinationSize - 2;
  double bucketSize = static_cast<double>(sourceSize - 2) /
                      static_cast<double>(totalInnerBuckets);

  int threadCount = QThread::idealThreadCount();
  if (threadCount < 1) {
    threadCount = 1;
  }

  qsizetype bucketsPerThread = totalInnerBuckets / threadCount;
  QFutureSynchronizer<void> synchronizer;

  for (int t = 0; t < threadCount; ++t) {
    qsizetype startBucket = t * bucketsPerThread;
    qsizetype endBucket = (t == threadCount - 1)
                              ? totalInnerBuckets
                              : startBucket + bucketsPerThread;

    if (startBucket >= endBucket) {
      continue;
    }

    synchronizer.addFuture(QtConcurrent::run([=]() {
      // Approximate Point A for the start of the chunk
      qsizetype aIndex = static_cast<qsizetype>(startBucket * bucketSize);
      if (aIndex < 0) {
        aIndex = 0;
      }

      for (qsizetype i = startBucket; i < endBucket; ++i) {
        // Calculate Point C (Average of next bucket)
        qreal avgX = 0;
        qreal avgY = 0;
        qsizetype avgRangeStart =
            static_cast<qsizetype>((i + 1) * bucketSize) + 1;
        qsizetype avgRangeEnd =
            static_cast<qsizetype>((i + 2) * bucketSize) + 1;

        if (avgRangeEnd >= sourceSize) {
          avgRangeEnd = sourceSize - 1;
        }
        qsizetype avgRangeLength = avgRangeEnd - avgRangeStart;

        if (avgRangeLength > 0) {
          for (qsizetype j = avgRangeStart; j < avgRangeEnd; ++j) {
            avgX += source[j].x();
            avgY += source[j].y();
          }
          avgX /= avgRangeLength;
          avgY /= avgRangeLength;
        }

        qreal pointCX = avgX;
        qreal pointCY = avgY;

        qsizetype rangeFrom = static_cast<qsizetype>(i * bucketSize) + 1;
        qsizetype rangeTo = static_cast<qsizetype>((i + 1) * bucketSize) + 1;
        if (rangeTo >= sourceSize) {
          rangeTo = sourceSize - 1;
        }

        qreal pointAX = source[aIndex].x();
        qreal pointAY = source[aIndex].y();

        qreal constX = pointAX - pointCX;
        qreal constY = pointCY - pointAY;

        qreal maxArea = -1.0;
        qsizetype nextAIndex = rangeFrom;

        for (qsizetype j = rangeFrom; j < rangeTo; ++j) {
          qreal area = std::abs((constX * (source[j].y() - pointAY)) -
                                ((pointAX - source[j].x()) * constY));

          if (area > maxArea) {
            maxArea = area;
            nextAIndex = j;
          }
        }

        destination[i + 1] = source[nextAIndex];
        aIndex = nextAIndex;
      }
    }));
  }

  synchronizer.waitForFinished();
}

} // namespace ChartPlotter

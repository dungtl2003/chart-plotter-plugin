#include "ChartPlotter/downsample/LargestTriangleThreeBuckets.hpp"

#include <QFutureSynchronizer>
#include <QThread>
#include <QtConcurrent>

namespace ChartPlotter {

void LargestTriangleThreeBuckets::downsample(
    QVector<QPointF>::const_iterator source, qsizetype sourceSize,
    QVector<QPointF>::iterator destination, qsizetype destinationSize) const {
  if (destinationSize == 0 || sourceSize == 0) {
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

  // Don't count first and last point
  qsizetype totalInnerBuckets = destinationSize - 2;
  double bucketSize = static_cast<double>(sourceSize - 2) /
                      static_cast<double>(destinationSize - 2);

  // Always add the first point and last point
  destination[0] = source[0];
  destination[destinationSize - 1] = source[sourceSize - 1];

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
        // Calculate point average for next bucket
        qreal avgX = 0;
        qreal avgY = 0;
        qsizetype avgRangeStart =
            static_cast<qsizetype>((i + 1) * bucketSize) + 1;
        qsizetype avgRangeEnd =
            static_cast<qsizetype>((i + 2) * bucketSize) + 1;
        if (avgRangeEnd > sourceSize) {
          avgRangeEnd = sourceSize;
        }

        qsizetype avgRangeLength = avgRangeEnd - avgRangeStart;

        for (; avgRangeStart < avgRangeEnd; ++avgRangeStart) {
          avgX += source[avgRangeStart].x();
          avgY += source[avgRangeStart].y();
        }
        avgX /= avgRangeLength;
        avgY /= avgRangeLength;

        qreal pointCX = avgX;
        qreal pointCY = avgY;

        // Get the range for this bucket
        qsizetype rangeFrom = static_cast<qsizetype>(i * bucketSize) + 1;
        qsizetype rangeTo = static_cast<qsizetype>((i + 1) * bucketSize) + 1;
        qreal pointAX = source[aIndex].x();
        qreal pointAY = source[aIndex].y();

        qreal constX = pointAX - pointCX;
        qreal constY = pointCY - pointAY;

        qreal maxArea = -1;
        qsizetype nextAIndex = 0;

        for (; rangeFrom < rangeTo; ++rangeFrom) {
          /**
           * Calculate triangle area over three buckets.
           *
           * Formular:
           *
           * Area = 1/2 * |(xa - xc)(yb - ya) - (xa - xb)(yc - ya)|
           */
          qreal area = std::abs((constX * (source[rangeFrom].y() - pointAY)) -
                                ((pointAX - source[rangeFrom].x()) *
                                 constY)); // source[rangeFrom].x() = pointBX,
                                           // source[rangeFrom].y() = pointBY,
                                           // we use this directly to reduce ops
          // we only care about which is bigger, so no need to divide with 2
          if (area > maxArea) {
            maxArea = area;
            nextAIndex = rangeFrom; // Next a is this b
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

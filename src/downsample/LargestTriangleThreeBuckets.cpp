#include "ChartPlotter/downsample/LargestTriangleThreeBuckets.hpp"

namespace ChartPlotter {

void LargestTriangleThreeBuckets::downsample(
    QVector<QPointF>::iterator source, qsizetype sourceSize,
    QVector<QPointF>::iterator destination, qsizetype destinationSize) {
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
  double bucketSize = static_cast<double>(sourceSize - 2) /
                      static_cast<double>(destinationSize - 2);

  // Always add the first point
  *destination = *source;
  ++destination;

  qsizetype aIndex = 0; // Initially a is the first point in the triangle

  for (qsizetype i = 0; i < destinationSize - 2; ++i) {
    // Calculate point average for next bucket
    qreal avgX = 0;
    qreal avgY = 0;
    qsizetype avgRangeStart = static_cast<qsizetype>((i + 1) * bucketSize) + 1;
    qsizetype avgRangeEnd = static_cast<qsizetype>((i + 2) * bucketSize) + 1;
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

    qreal maxArea = -1;
    qsizetype nextAIndex = 0;

    for (; rangeFrom < rangeTo; ++rangeFrom) {
      qreal pointBX = source[rangeFrom].x();
      qreal pointBY = source[rangeFrom].y();

      /**
       * Calculate triangle area over three buckets.
       *
       * Formular:
       *
       * Area = 1/2 * |(xa - xc)(yb - ya) - (xa - xb)(yc - ya)|
       */
      qreal area = std::abs(((pointAX - pointCX) * (pointBY - pointAY)) -
                            ((pointAX - pointBX) * (pointCY - pointAY)));
      // we only care about which is bigger, so no need to divide with 2
      if (area > maxArea) {
        maxArea = area;
        nextAIndex = rangeFrom; // Next a is this b
      }
    }

    *destination = source[nextAIndex];
    ++destination;
    aIndex = nextAIndex; // This a is the next a (chosen b)
  }

  *destination = source[sourceSize - 1]; // Always add last
  ++destination;
}

} // namespace ChartPlotter

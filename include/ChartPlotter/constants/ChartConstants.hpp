#pragma once

#include <QtQml>

namespace ChartPlotter {

class ChartConstants : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(int LINE_STROKE_WIDTH_MIN READ lineStrokeMinWidth CONSTANT)
  Q_PROPERTY(int LINE_STROKE_WIDTH_MAX READ lineStrokeMaxWidth CONSTANT)
  Q_PROPERTY(int LINE_AA_MIN READ lineMinAA CONSTANT)
  Q_PROPERTY(int LINE_AA_MAX READ lineMaxAA CONSTANT)

public:
  explicit ChartConstants(QObject *parent = nullptr) : QObject(parent) {}

  static constexpr qreal LINE_STROKE_WIDTH_MIN = 1;
  static constexpr qreal LINE_STROKE_WIDTH_MAX = 10;
  static constexpr qreal LINE_AA_MIN = 0;
  static constexpr qreal LINE_AA_MAX = 4;
  static constexpr double EPSILON = 1e-6;

  int lineStrokeMinWidth() const { return LINE_STROKE_WIDTH_MIN; }
  int lineStrokeMaxWidth() const { return LINE_STROKE_WIDTH_MAX; }
  int lineMinAA() const { return LINE_AA_MIN; }
  int lineMaxAA() const { return LINE_AA_MAX; }
};

} // namespace ChartPlotter

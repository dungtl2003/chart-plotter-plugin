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
  Q_PROPERTY(int TICK_COUNT_MAX READ maxTickCount CONSTANT)
  Q_PROPERTY(int TICK_COUNT_MIN READ minTickCount CONSTANT)
  Q_PROPERTY(int FPS_MIN READ fpsMin CONSTANT)
  Q_PROPERTY(int FPS_MAX READ fpsMax CONSTANT)

public:
  explicit ChartConstants(QObject *parent = nullptr) : QObject(parent) {}

  static constexpr float LINE_STROKE_WIDTH_MIN = 1;
  static constexpr float LINE_STROKE_WIDTH_MAX = 10;
  // Bars auto-size to their band: each bar fills this fraction of its slot, never
  // drawn thinner than the floor (so dense charts stay visible).
  static constexpr float BAR_BAND_FILL = 0.8;
  static constexpr float BAR_MIN_PIXEL_WIDTH = 1;
  static constexpr float LINE_AA_MIN = 0;
  static constexpr float LINE_AA_MAX = 4;
  static constexpr double EPSILON = 1e-6;
  // Deepest zoom-in: the visible window may not shrink below this fraction of the
  // full data span, nor (for large-magnitude values like epoch ms) below this
  // fraction of the values' magnitude. Stops infinite zoom that collapses the
  // axis into many identical-labelled ticks and divides the tick step to zero.
  static constexpr double MIN_ZOOM_SPAN_FRACTION = 1e-4;
  static constexpr double MIN_ZOOM_MAGNITUDE_FRACTION = 1e-9;
  // Hard cap on generated ticks — a defensive guard against a degenerate
  // (near-zero) step producing an unbounded tick loop.
  static constexpr int MAX_AXIS_TICKS = 1000;
  // Smallest usable plot area (pixels) on either axis. Below this the window has
  // been shrunk so far that the plot rect goes zero/negative; rendering it would
  // divide by a ~0 width and crash, so we skip the frame instead.
  static constexpr double MIN_PLOT_SIZE = 4.0;
  static constexpr int TICK_COUNT_MAX = 20;
  static constexpr int TICK_COUNT_MIN = 2;
  static constexpr int FPS_MIN = 1;
  static constexpr int FPS_MAX = 240;

  // Length of the rolling window over which the debug-only render performance
  // monitor (frameSwapped-based FPS + resident memory) aggregates and logs.
  static constexpr int PERF_REPORT_INTERVAL_MS = 1000;

  int lineStrokeMinWidth() const { return LINE_STROKE_WIDTH_MIN; }
  int lineStrokeMaxWidth() const { return LINE_STROKE_WIDTH_MAX; }
  int lineMinAA() const { return LINE_AA_MIN; }
  int lineMaxAA() const { return LINE_AA_MAX; }
  int maxTickCount() const { return TICK_COUNT_MAX; }
  int minTickCount() const { return TICK_COUNT_MIN; }
  int fpsMin() const { return FPS_MIN; }
  int fpsMax() const { return FPS_MAX; }
};

} // namespace ChartPlotter

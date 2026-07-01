#include "ChartPlotter/strategy/PieSeriesStrategy.hpp"

#include "ChartPlotter/data/PieRenderData.hpp"
#include "ChartPlotter/series/PieSeries.hpp"
#include "ChartPlotter/utils/LoggerManager.hpp"

#include <QtMath>

#include <cmath>

namespace ChartPlotter {

namespace {

// Categorical palette (Tableau 10). Slices cycle through it by index, so a pie
// with more entries than colours simply repeats — good enough for the simple
// single-series pie.
const QColor kPalette[] = {
    QColor("#4e79a7"), QColor("#f28e2b"), QColor("#e15759"),
    QColor("#76b7b2"), QColor("#59a14f"), QColor("#edc948"),
    QColor("#b07aa1"), QColor("#ff9da7"), QColor("#9c755f"),
    QColor("#bab0ac"),
};
constexpr int kPaletteSize =
    static_cast<int>(sizeof(kPalette) / sizeof(kPalette[0]));

} // namespace

std::unique_ptr<RenderData>
PieSeriesStrategy::build(const AbstractSeries &series,
                         const ResolvedSeriesData &resolved,
                         const DataSnapshot &snapshot,
                         SeriesBuildContext & /*context*/) {
  auto data = std::make_unique<PieRenderData>();

  if (!resolved.valid) {
    CP_WARN("PieSeriesStrategy::build: resolved series is invalid: {}",
            resolved.errorMessage.toStdString());
    return data;
  }

  const int labelIndex = resolved.labelColumnIndex;
  const int valueIndex = resolved.valueColumnIndex;

  if (labelIndex < 0 || labelIndex >= snapshot.columnCount || valueIndex < 0 ||
      valueIndex >= snapshot.columnCount) {
    CP_WARN("PieSeriesStrategy::build: invalid column index");
    return data;
  }

  if (resolved.valueColumnType != ChartEnums::DataType::Number) {
    CP_WARN("PieSeriesStrategy::build: value column must be Number");
    return data;
  }

  const auto *pieSeries = qobject_cast<const PieSeries *>(&series);
  if (!pieSeries) {
    CP_WARN("PieSeriesStrategy::build: series is not PieSeries");
    return data;
  }

  // Optional per-slice colour overrides (by slice index); palette fills the rest.
  const QVariantList overrideColors = pieSeries->colors();

  const bool labelIsCategory =
      resolved.labelColumnType == ChartEnums::DataType::String;

  struct RawSlice {
    QString label;
    double value;
  };
  QVector<RawSlice> raw;
  raw.reserve(snapshot.rowCount);

  double total = 0.0;
  for (qint64 row = 0; row < snapshot.rowCount; ++row) {
    const double value = snapshot.valueAt(valueIndex, row);

    // Negative or non-finite values have no meaning in a pie; drop them.
    if (std::isnan(value) || !std::isfinite(value) || value <= 0.0) {
      continue;
    }

    const double rawLabel = snapshot.valueAt(labelIndex, row);
    const QString label = labelIsCategory
                              ? snapshot.categoryName(labelIndex, rawLabel)
                              : QString::number(rawLabel);

    raw.push_back({label, value});
    total += value;
  }

  if (raw.isEmpty() || total <= 0.0) {
    CP_WARN("PieSeriesStrategy::build: no positive values to plot");
    // Still valid (empty) so the chart can render a blank plot area.
    data->valid = true;
    return data;
  }

  // Normalise by the total so the slices always fill the full circle. When the
  // input already sums to 100 (the simple percentage case) each share equals
  // the raw value; otherwise it is scaled to a fraction of 2*pi.
  data->slices.reserve(raw.size());
  double startAngle = 0.0;
  for (int i = 0; i < raw.size(); ++i) {
    const double fraction = raw[i].value / total;
    const double sweep = fraction * 2.0 * M_PI;

    QColor color = kPalette[i % kPaletteSize];
    if (i < overrideColors.size()) {
      // A QML `color` arrives as a QColor variant; a hex string (e.g. from the
      // settings dialog) as a QString. Accept either.
      QColor picked = overrideColors[i].value<QColor>();
      if (!picked.isValid()) {
        picked = QColor(overrideColors[i].toString());
      }
      if (picked.isValid()) {
        color = picked;
      }
    }

    PieSlice slice;
    slice.startAngle = startAngle;
    slice.sweepAngle = sweep;
    slice.color = color;
    slice.label = raw[i].label;
    slice.value = raw[i].value;
    slice.percentage = fraction * 100.0;

    data->slices.push_back(std::move(slice));
    startAngle += sweep;
  }

  data->valid = true;
  return data;
}

} // namespace ChartPlotter

#include "ChartPlotter/utils/Variant.hpp"

#include <QTimeZone>

namespace ChartPlotter {

bool Utils::Variant::isDouble(const QVariant &value) {
  bool ok = false;
  const double result = value.toDouble(&ok);

  if (!ok || !std::isfinite(result)) {
    return false;
  }

  return true;
}

bool Utils::Variant::isDate(const QVariant &value) {
  return value.toDateTime().isValid() || value.toDate().isValid();
}

bool Utils::Variant::variantToDouble(const QVariant &value, double &out) {
  bool ok = false;
  const double result = value.toDouble(&ok);

  if (!ok || !std::isfinite(result)) {
    return false;
  }

  out = result;
  return true;
}

bool Utils::Variant::variantToDateNumber(const QVariant &value, double &out) {
  if (value.canConvert<QDateTime>()) {
    QDateTime dt = value.toDateTime();

    if (dt.isValid()) {
      // Reinterpret a zone-less (LocalTime) wall-clock as UTC so date values
      // agree with the CSV fast-path parsers and the UTC-formatted axis labels.
      if (dt.timeSpec() == Qt::LocalTime) {
        dt.setTimeZone(QTimeZone::UTC);
      }
      out = static_cast<double>(dt.toMSecsSinceEpoch());
      return true;
    }
  }

  if (value.canConvert<QDate>()) {
    const QDate date = value.toDate();

    if (date.isValid()) {
      QDateTime dt(date.startOfDay());
      dt.setTimeZone(QTimeZone::UTC);
      out = static_cast<double>(dt.toMSecsSinceEpoch());
      return true;
    }
  }

  return variantToDouble(value, out);
}

} // namespace ChartPlotter

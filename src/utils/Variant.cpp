#include "ChartPlotter/utils/Variant.hpp"

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
    const QDateTime dt = value.toDateTime();

    if (dt.isValid()) {
      out = static_cast<double>(dt.toMSecsSinceEpoch());
      return true;
    }
  }

  if (value.canConvert<QDate>()) {
    const QDate date = value.toDate();

    if (date.isValid()) {
      out =
          static_cast<double>(QDateTime(date.startOfDay()).toMSecsSinceEpoch());
      return true;
    }
  }

  return variantToDouble(value, out);
}

} // namespace ChartPlotter

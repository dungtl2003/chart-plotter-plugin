#include "ChartPlotter/utils/Parse.hpp"

namespace ChartPlotter {

bool Parse::isInt(const QString &value) {
  bool ok = false;
  value.toInt(&ok);
  return ok;
}

bool Parse::isDouble(const QString &value) {
  bool ok = false;
  value.toDouble(&ok);
  return ok;
}

bool Parse::isDate(const QString &value) {
  const QString s = value.trimmed();

  return QDate::fromString(s, Qt::ISODate).isValid() ||
         QDate::fromString(s, "yyyy-MM-dd").isValid() ||
         QDate::fromString(s, "dd/MM/yyyy").isValid() ||
         QDate::fromString(s, "MM/dd/yyyy").isValid();
}

bool Parse::isBool(const QString &value) {
  const QString s = value.trimmed().toLower();

  return s == "true" || s == "false" || s == "1" || s == "0" || s == "yes" ||
         s == "no" || s == "y" || s == "n";
}

double Parse::toInt(const QString &value) {
  bool ok = false;
  const int result = value.toInt(&ok);

  return ok ? result : 0;
}

double Parse::toDouble(const QString &value) {
  bool ok = false;
  const double result = value.toDouble(&ok);

  return ok ? result : 0.0;
}

QDate Parse::toDate(const QString &value) {
  const QString s = value.trimmed();

  QDate date = QDate::fromString(s, Qt::ISODate);
  if (date.isValid())
    return date;

  date = QDate::fromString(s, "yyyy-MM-dd");
  if (date.isValid())
    return date;

  date = QDate::fromString(s, "dd/MM/yyyy");
  if (date.isValid())
    return date;

  date = QDate::fromString(s, "MM/dd/yyyy");
  if (date.isValid())
    return date;

  return {};
}

bool Parse::toBool(const QString &value) {
  const QString s = value.trimmed().toLower();

  return s == "true" || s == "1" || s == "yes" || s == "y";
}

} // namespace ChartPlotter

#pragma once

#include <QDateTime>
#include <QString>

namespace ChartPlotter {

namespace Parse {

bool isInt(const QString &value);
bool isDouble(const QString &value);
bool isDate(const QString &value);
bool isBool(const QString &value);

double toInt(const QString &value);
double toDouble(const QString &value);
QDate toDate(const QString &value);
bool toBool(const QString &value);

} // namespace Parse

} // namespace ChartPlotter

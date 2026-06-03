#pragma once

#include <QDateTime>
#include <QString>

namespace ChartPlotter {

namespace Utils::Variant {

bool isDouble(const QVariant &value);
bool isDate(const QVariant &value);

bool variantToDouble(const QVariant &value, double &out);
bool variantToDateNumber(const QVariant &value, double &out);

} // namespace Utils::Variant

} // namespace ChartPlotter

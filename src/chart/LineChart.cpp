#include "ChartPlotter/chart/LineChart.hpp"

LineChart::LineChart(QObject *parent) : AbstractChart(parent) {}

ChartType LineChart::type() const { return ChartType::Line; }

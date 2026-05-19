#include "ChartPlotter/chart/BarChart.hpp"

BarChart::BarChart(QObject *parent) : AbstractChart(parent) {}

ChartType BarChart::type() const { return ChartType::Bar; }

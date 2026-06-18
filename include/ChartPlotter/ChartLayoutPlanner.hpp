#pragma once

#include "ChartPlotter/series/AbstractSeries.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"

#include <QString>

namespace ChartPlotter {

class ChartLayoutPolicy {
public:
  ChartLayoutPolicy() = default;

  explicit ChartLayoutPolicy(const ChartLayoutPolicy &) = delete;
  explicit ChartLayoutPolicy(ChartLayoutPolicy &&) = delete;
  ChartLayoutPolicy &operator=(const ChartLayoutPolicy &) = delete;
  ChartLayoutPolicy &operator=(ChartLayoutPolicy &&) = delete;
  virtual ~ChartLayoutPolicy() = default;

  virtual std::pair<bool, QString>
  apply(const QVector<QPointer<AbstractSeries>> &series) const = 0;
};

class XYAndPieMixNotAllowedPolicy : public ChartLayoutPolicy {
public:
  std::pair<bool, QString>
  apply(const QVector<QPointer<AbstractSeries>> &series) const override;
};

class LineAndBarMixNotAllowedPolicy : public ChartLayoutPolicy {
public:
  std::pair<bool, QString>
  apply(const QVector<QPointer<AbstractSeries>> &series) const override;
};

class MultipleBarSeriesNotAllowedPolicy : public ChartLayoutPolicy {
public:
  std::pair<bool, QString>
  apply(const QVector<QPointer<AbstractSeries>> &series) const override;
};

class MultiplePieSeriesNotAllowedPolicy : public ChartLayoutPolicy {
public:
  std::pair<bool, QString>
  apply(const QVector<QPointer<AbstractSeries>> &series) const override;
};

class SeriesNotEmptyPolicy : public ChartLayoutPolicy {
public:
  std::pair<bool, QString>
  apply(const QVector<QPointer<AbstractSeries>> &series) const override;
};

enum class CoordinateSystem {
  Unknown,
  Cartesian,
  Pie,
};

struct ChartMargins {
  qreal left = 90;
  qreal top = 90;
  qreal right = 90;
  qreal bottom = 90;
};

struct ChartChromeRequest {
  bool hasTitle = false;
  ChartEnums::LegendPosition legendPosition = ChartEnums::LegendPosition::None;
};

struct ChartLayoutPlan {

  bool valid = false;
  QString errorMessage;

  CoordinateSystem coordinateSystem = CoordinateSystem::Unknown;
  QVector<int> xySeriesIndexes;
  QVector<int> pieSeriesIndexes;

  ChartMargins plotMargins;

  bool hasTitle = false; // reserve a band at the top
  ChartEnums::LegendPosition legendPosition = ChartEnums::LegendPosition::None;
};

class ChartLayoutPlanner {

public:
  ChartLayoutPlanner();

  ChartLayoutPlan buildPlan(const QVector<QPointer<AbstractSeries>> &series,
                            const ChartChromeRequest &chrome = {}) const;

private:
  std::vector<std::unique_ptr<ChartLayoutPolicy>> m_policies;

  static CoordinateSystem coordinateSystemOf(const AbstractSeries *series);
  void applyDefaultPolicies();
};

} // namespace ChartPlotter

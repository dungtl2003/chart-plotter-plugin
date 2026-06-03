#pragma once

#include "ChartPlotter/data/DataSource.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"

#include <QObject>
#include <QtQml>

namespace ChartPlotter {

class AbstractSeries : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("AbstractSeries is a base class and cannot be instantiated.")

  Q_PROPERTY(ChartPlotter::DataSource *source READ source WRITE setSource NOTIFY
                 sourceChanged)

public:
  explicit AbstractSeries(QObject *parent = nullptr);

  explicit AbstractSeries(const AbstractSeries &) = delete;
  explicit AbstractSeries(AbstractSeries &&) = delete;
  AbstractSeries &operator=(const AbstractSeries &) = delete;
  AbstractSeries &operator=(AbstractSeries &&) = delete;
  virtual ~AbstractSeries() = default;

  virtual ChartEnums::SeriesType type() = 0;

  QPointer<DataSource> source() const;
  void setSource(QPointer<DataSource> newSource);

signals:
  void sourceChanged();

private:
  QPointer<DataSource> m_source;
};

} // namespace ChartPlotter

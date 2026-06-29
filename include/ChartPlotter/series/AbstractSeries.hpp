#pragma once

#include "ChartPlotter/data/DataSource.hpp"
#include "ChartPlotter/types/ChartEnums.hpp"

#include <QColor>
#include <QObject>
#include <QtQml>

namespace ChartPlotter {

class AbstractSeries : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("AbstractSeries is a base class and cannot be instantiated.")

  Q_PROPERTY(ChartPlotter::DataSource *source READ source WRITE setSource NOTIFY
                 sourceChanged)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY(ChartPlotter::ChartEnums::SeriesType seriesType READ type CONSTANT)

public:
  explicit AbstractSeries(QObject *parent = nullptr);

  explicit AbstractSeries(const AbstractSeries &) = delete;
  explicit AbstractSeries(AbstractSeries &&) = delete;
  AbstractSeries &operator=(const AbstractSeries &) = delete;
  AbstractSeries &operator=(AbstractSeries &&) = delete;
  virtual ~AbstractSeries() = default;

  virtual ChartEnums::SeriesType type() const = 0;
  virtual QColor legendColor() const;

  QPointer<DataSource> source() const;
  void setSource(QPointer<DataSource> newSource);

  QString name() const;
  void setName(const QString &name);

signals:
  void nameChanged();
  void sourceChanged();

private:
  QString m_name;
  QPointer<DataSource> m_source;
};

} // namespace ChartPlotter

#pragma once

#include "ChartPlotter/series/AbstractSeries.hpp"

namespace ChartPlotter {

class PieSeries : public AbstractSeries {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)

public:
  explicit PieSeries(QObject *parent = nullptr);

  QString label() const;
  void setLabel(const QString &newLabel);
  QString value() const;
  void setValue(const QString &newValue);

  ChartEnums::SeriesType type() override;

signals:
  void labelChanged();
  void valueChanged();

private:
  QString m_label;
  QString m_value;
};

} // namespace ChartPlotter

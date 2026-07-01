#pragma once

#include "ChartPlotter/series/AbstractSeries.hpp"

#include <QVariantList>

namespace ChartPlotter {

class PieSeries : public AbstractSeries {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
  // Per-slice colours, indexed by slice order. Slices past the end of this list
  // fall back to the built-in palette. Accepts color values or hex strings.
  Q_PROPERTY(QVariantList colors READ colors WRITE setColors NOTIFY colorsChanged)

public:
  explicit PieSeries(QObject *parent = nullptr);

  QString label() const;
  void setLabel(const QString &newLabel);
  QString value() const;
  void setValue(const QString &newValue);

  QVariantList colors() const;
  void setColors(const QVariantList &newColors);

  ChartEnums::SeriesType type() const override;

signals:
  void labelChanged();
  void valueChanged();
  void colorsChanged();

private:
  QString m_label;
  QString m_value;
  QVariantList m_colors;
};

} // namespace ChartPlotter

#pragma once

#include "ChartPlotter/series/XYSeries.hpp"

#include <QColor>

namespace ChartPlotter {

class LineSeries : public XYSeries {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY
                 strokeColorChanged)
  Q_PROPERTY(float strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY
                 strokeWidthChanged)
  Q_PROPERTY(float strokeMiterLimit READ strokeMiterLimit WRITE
                 setStrokeMiterLimit NOTIFY strokeMiterLimitChanged)
  Q_PROPERTY(
      ChartPlotter::ChartEnums::StrokePattern strokePattern READ strokePattern
          WRITE setStrokePattern NOTIFY strokeMiterLimitChanged)

  Q_PROPERTY(bool markerVisible READ markerVisible WRITE setMarkerVisible NOTIFY
                 markerVisibleChanged)
  Q_PROPERTY(QColor markerColor READ markerColor WRITE setMarkerColor NOTIFY
                 markerColorChanged)

  Q_PROPERTY(
      float antialias READ antialias WRITE setAntialias NOTIFY antialiasChanged)

public:
  explicit LineSeries(QObject *parent = nullptr);

  ChartEnums::SeriesType type() override;

  QColor strokeColor() const;
  void setStrokeColor(const QColor &color);

  float strokeWidth() const;
  void setStrokeWidth(float width);

  float strokeMiterLimit() const;
  void setStrokeMiterLimit(float newLimit);

  ChartEnums::StrokePattern strokePattern() const;
  void setStrokePattern(ChartEnums::StrokePattern newPattern);

  bool markerVisible() const;
  void setMarkerVisible(bool visible);

  QColor markerColor() const;
  void setMarkerColor(const QColor &color);

  float antialias() const;
  void setAntialias(float antialias);

signals:
  void strokeColorChanged();
  void strokeWidthChanged();
  void strokeMiterLimitChanged();
  void strokePatternChanged();

  void markerVisibleChanged();
  void markerColorChanged();

  void antialiasChanged();

private:
  QColor m_strokeColor = QColor("#ff3333");
  float m_strokeWidth = 5.0f;
  float m_strokeMiterLimit = 4.0f;
  ChartEnums::StrokePattern m_strokePattern = ChartEnums::StrokePattern::Solid;

  bool m_markerVisible = false;
  QColor m_markerColor = QColor("#000000");

  float m_antialias = 1.0f;
};

} // namespace ChartPlotter

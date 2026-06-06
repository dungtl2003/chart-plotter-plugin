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

  Q_PROPERTY(bool markerVisible READ markerVisible WRITE setMarkerVisible NOTIFY
                 markerVisibleChanged)
  Q_PROPERTY(QColor markerColor READ markerColor WRITE setMarkerColor NOTIFY
                 markerColorChanged)
  Q_PROPERTY(float markerRadius READ markerRadius WRITE setMarkerRadius NOTIFY
                 markerRadiusChanged)

  Q_PROPERTY(
      float antialias READ antialias WRITE setAntialias NOTIFY antialiasChanged)

public:
  explicit LineSeries(QObject *parent = nullptr);

  ChartEnums::SeriesType type() override;

  QColor strokeColor() const;
  void setStrokeColor(const QColor &color);

  float strokeWidth() const;
  void setStrokeWidth(float width);

  bool markerVisible() const;
  void setMarkerVisible(bool visible);

  QColor markerColor() const;
  void setMarkerColor(const QColor &color);

  float markerRadius() const;
  void setMarkerRadius(float radius);

  float antialias() const;
  void setAntialias(float antialias);

signals:
  void strokeColorChanged();
  void strokeWidthChanged();

  void markerVisibleChanged();
  void markerColorChanged();
  void markerRadiusChanged();

  void antialiasChanged();

private:
  QColor m_strokeColor = QColor("#ff3333");
  float m_strokeWidth = 1.0f;

  bool m_markerVisible = false;
  QColor m_markerColor = QColor("#000000");
  float m_markerRadius = 1.0f;

  float m_antialias = 1.0f;
};

} // namespace ChartPlotter

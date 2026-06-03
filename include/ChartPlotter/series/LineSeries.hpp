#pragma once

#include "ChartPlotter/series/XYSeries.hpp"

#include <QColor>

namespace ChartPlotter {

class LineSeries : public XYSeries {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY
                 strokeColorChanged)
  Q_PROPERTY(int strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY
                 strokeWidthChanged)

  Q_PROPERTY(bool markerVisible READ markerVisible WRITE setMarkerVisible NOTIFY
                 markerVisibleChanged)
  Q_PROPERTY(QColor markerColor READ markerColor WRITE setMarkerColor NOTIFY
                 markerColorChanged)
  Q_PROPERTY(int markerRadius READ markerRadius WRITE setMarkerRadius NOTIFY
                 markerRadiusChanged)

  Q_PROPERTY(
      int antialias READ antialias WRITE setAntialias NOTIFY antialiasChanged)

public:
  explicit LineSeries(QObject *parent = nullptr);

  ChartEnums::SeriesType type() override;

  QColor strokeColor() const;
  void setStrokeColor(const QColor &color);

  int strokeWidth() const;
  void setStrokeWidth(int width);

  bool markerVisible() const;
  void setMarkerVisible(bool visible);

  QColor markerColor() const;
  void setMarkerColor(const QColor &color);

  int markerRadius() const;
  void setMarkerRadius(int radius);

  int antialias() const;
  void setAntialias(int antialias);

signals:
  void strokeColorChanged();
  void strokeWidthChanged();

  void markerVisibleChanged();
  void markerColorChanged();
  void markerRadiusChanged();

  void antialiasChanged();

private:
  QColor m_strokeColor = QColor("#ff3333");
  int m_strokeWidth = 1;

  bool m_markerVisible = false;
  QColor m_markerColor = QColor("#000000");
  int m_markerRadius = 1;

  int m_antialias = 1;
};

} // namespace ChartPlotter

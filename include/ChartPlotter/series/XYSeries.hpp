#pragma once

#include "ChartPlotter/series/AbstractSeries.hpp"

namespace ChartPlotter {

enum class ColumnBindingKind { Name, Index, Invalid };

struct ColumnBinding {
  ColumnBindingKind kind = ColumnBindingKind::Invalid;
  QString name;
  qint64 index = -1;

  static ColumnBinding byName(const QString &name) {
    ColumnBinding binding;
    binding.kind =
        name.isEmpty() ? ColumnBindingKind::Invalid : ColumnBindingKind::Name;
    binding.name = name;
    return binding;
  }

  static ColumnBinding byIndex(int index) {
    ColumnBinding binding;
    binding.kind =
        index >= 0 ? ColumnBindingKind::Index : ColumnBindingKind::Invalid;
    binding.index = index;
    return binding;
  }
};

class XYSeries : public AbstractSeries {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("XYSeries is a base class and cannot be instantiated.")

  Q_PROPERTY(QString x READ x WRITE setX NOTIFY xChanged)
  Q_PROPERTY(QString y READ y WRITE setY NOTIFY yChanged)
  Q_PROPERTY(int xColumn READ xColumn WRITE setXColumn NOTIFY xColumnChanged)
  Q_PROPERTY(int yColumn READ yColumn WRITE setYColumn NOTIFY yColumnChanged)

public:
  explicit XYSeries(QObject *parent = nullptr);

  QString x() const;
  void setX(const QString &newX);
  QString y() const;
  void setY(const QString &newY);
  int xColumn() const;
  void setXColumn(int newXColumn);
  int yColumn() const;
  void setYColumn(int newYColumn);

  // priorty index over name
  ColumnBinding xBinding() const;
  ColumnBinding yBinding() const;

signals:
  void xChanged();
  void yChanged();
  void xColumnChanged();
  void yColumnChanged();

private:
  QString m_x;
  QString m_y;
  int m_xColumn = -1;
  int m_yColumn = -1;
};

} // namespace ChartPlotter

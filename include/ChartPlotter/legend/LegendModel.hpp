#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QVector>
#include <QtQml>

namespace ChartPlotter {

struct LegendEntry {
  QString name;
  QColor color;
  bool visible = true;
};

class LegendModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE(
      "LegendModel should be created in C++ and passed as a property")

public:
  enum Roles {
    NameRole = Qt::UserRole + 1,
    ColorRole,
    VisibleRole,
  };

  explicit LegendModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  void setEntries(const QVector<LegendEntry> &entries);
  bool isVisible(int row) const;

  Q_INVOKABLE void setSeriesVisible(int row, bool visible);
  Q_INVOKABLE void toggleSeries(int row);

signals:
  void visibilityChanged(int row, bool visible);

private:
  QVector<LegendEntry> m_entries;
};

} // namespace ChartPlotter
